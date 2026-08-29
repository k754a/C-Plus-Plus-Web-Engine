'use client';

import { useEffect, useRef, useState, useSyncExternalStore } from 'react';
import { io, type Socket } from 'socket.io-client';

// ----- Types matching the C++ engine JSON protocol -----
type LayoutItem = {
  type: string; // "text" | "image"
  text?: string;
  src?: string;
  x: number;
  y: number;
  width?: number;
  height?: number;
  fontSize?: number;
  color?: string;
  href?: string;
  bgColor?: string;
};

type Layout = {
  title?: string;
  bgColor?: string;
  items?: LayoutItem[];
  error?: string;
};

type Tab = {
  id: number;
  title: string;
  url: string; // current location ("home" or a URL/search query)
  history: string[];
  historyPos: number;
  layout: Layout | null;
};

// ----- Constants matching the Windows SDL3 GUI -----
const TAB_BAR_H = 30; // #d2d2d2
const NAV_BAR_H = 39; // #f5f5f5
const UI_H = TAB_BAR_H + NAV_BAR_H; // 69
const DEFAULT_BG = '#f5f5f5';
const SCROLL_W = 20;
const TAB_W = 180;
const TAB_GAP = 2;
const PX_FONT = "'PixelifySans', monospace";

let tabIdCounter = 1;

function makeTab(title: string, url: string): Tab {
  return {
    id: tabIdCounter++,
    title,
    url,
    history: [url],
    historyPos: 0,
    layout: null,
  };
}

function truncateTitle(title: string): string {
  if (title.length > 17) return title.slice(0, 17) + '...';
  return title;
}

// Subscribe to window height without triggering hydration mismatches.
function subscribeWinH(cb: () => void) {
  window.addEventListener('resize', cb);
  return () => window.removeEventListener('resize', cb);
}
function getWinH() {
  return window.innerHeight;
}
function getWinHServer() {
  return 800;
}

// CHANGED WITH AI: Subscribe to window WIDTH too — needed for the new
// horizontal scrollbar (canScrollH depends on viewport width vs content width).
function subscribeWinW(cb: () => void) {
  window.addEventListener('resize', cb);
  return () => window.removeEventListener('resize', cb);
}
function getWinW() {
  return window.innerWidth;
}
function getWinWServer() {
  return 1200;
}

export default function Home() {
  const [tabs, setTabs] = useState<Tab[]>(() => [makeTab('New Tab', 'home')]);
  const [activeTab, setActiveTab] = useState(0);
  const [urlInput, setUrlInput] = useState('');
  const [loadingTabId, setLoadingTabId] = useState<number | null>(null);
  const [bookmarks, setBookmarks] = useState<string[]>([]);
  const [scrollTop, setScrollTop] = useState(0);
  // CHANGED WITH AI: track horizontal scroll position for the new H-scrollbar.
  const [scrollLeft, setScrollLeft] = useState(0);
  const [cursorVisible, setCursorVisible] = useState(true);
  const [urlFocused, setUrlFocused] = useState(false);
  const [textWidth, setTextWidth] = useState(0);
  // CHANGED WITH AI: track which nav button is hovered for inline hover highlight
  const [hoveredBtn, setHoveredBtn] = useState<string | null>(null);
  const winH = useSyncExternalStore(subscribeWinH, getWinH, getWinHServer);
  // CHANGED WITH AI: track window width for the horizontal scrollbar.
  const winW = useSyncExternalStore(subscribeWinW, getWinW, getWinWServer);

  const socketRef = useRef<Socket | null>(null);
  const scrollRef = useRef<HTMLDivElement>(null);
  const navTabIdRef = useRef<number | null>(null);
  const urlMeasureRef = useRef<HTMLSpanElement>(null);
  // CHANGED WITH AI: dragState now tracks both vertical AND horizontal scrollbar
  // drags. `axis` is 'v' or 'h'. For 'v' we use startY/startScroll (scrollTop);
  // for 'h' we use startX/startScrollH (scrollLeft).
  const dragStateRef = useRef<
    | { axis: 'v'; startY: number; startScroll: number }
    | { axis: 'h'; startX: number; startScroll: number }
    | null
  >(null);
  const tabScrollRef = useRef<Map<number, number>>(new Map());
  const viewportHRef = useRef(800);
  const contentHeightRef = useRef(800);
  // CHANGED WITH AI: refs for the horizontal scroll range, used by the
  // window-level drag listener (which reads refs, not state).
  const viewportWRef = useRef(1200);
  const contentWidthRef = useRef(1200);

  const activeTabData = tabs[Math.min(activeTab, tabs.length - 1)] ?? tabs[0];
  const currentLayout = activeTabData?.layout ?? null;
  const currentUrl = activeTabData?.url ?? '';
  const currentBg = currentLayout?.bgColor || DEFAULT_BG;
  const isBookmarked =
    currentUrl !== '' && currentUrl !== 'home' && bookmarks.includes(currentUrl);

  const viewportH = Math.max(100, winH - UI_H);

  function computeContentHeight(): number {
    if (!currentLayout?.items?.length) return viewportH;
    let max = 0;
    for (const item of currentLayout.items) {
      const h = item.height || (item.fontSize ? item.fontSize + 4 : 20);
      max = Math.max(max, item.y + h);
    }
    return Math.max(viewportH, max + 100);
  }
  const contentHeight = computeContentHeight();

  // CHANGED WITH AI: viewport width for the content area. The vertical scrollbar
  // (SCROLL_W) eats into the available width when it's visible. Declared AFTER
  // contentHeight so we can check whether the vertical scrollbar will show.
  const viewportW = Math.max(100, winW - (contentHeight > viewportH ? SCROLL_W : 0));

  // CHANGED WITH AI: compute the widest layout item so we know if a horizontal
  // scrollbar is needed. Some pages (e.g. Bing search results) have wide text
  // items that overflow the viewport — without horizontal scroll, the right
  // side of those items is unreachable.
  function computeContentWidth(): number {
    if (!currentLayout?.items?.length) return viewportW;
    let max = 0;
    for (const item of currentLayout.items) {
      // for text items, width is already estimated by the engine; for images,
      // item.width is set. Fall back to a small value if neither is present.
      const itemW = item.width || (item.fontSize ? item.fontSize * 8 : 20);
      max = Math.max(max, item.x + itemW);
    }
    return Math.max(viewportW, max + 100);
  }
  const contentWidth = computeContentWidth();

  // Keep refs in sync with the latest viewport/content size for the
  // window-level drag listeners (which read refs, not state).
  useEffect(() => {
    viewportHRef.current = viewportH;
    contentHeightRef.current = contentHeight;
    viewportWRef.current = viewportW;
    contentWidthRef.current = contentWidth;
  }, [viewportH, contentHeight, viewportW, contentWidth]);

  const canScroll = contentHeight > viewportH;
  const barHeight = canScroll
    ? Math.max(20, (viewportH / contentHeight) * viewportH)
    : 0;
  const barTop =
    canScroll && contentHeight - viewportH > 0
      ? (scrollTop / (contentHeight - viewportH)) * (viewportH - barHeight)
      : 0;

  // CHANGED WITH AI: horizontal scrollbar metrics — mirror the vertical ones.
  // canScrollH is true when the widest layout item exceeds the viewport width.
  // The H-scrollbar lives at the bottom of the content area, with height = SCROLL_W
  // (same thickness as the vertical one). When the vertical scrollbar is also
  // visible, the H-scrollbar stops SCROLL_W px short of the right edge so the
  // two don't overlap (a small filler box occupies the bottom-right corner).
  const canScrollH = contentWidth > viewportW;
  const barWidth = canScrollH
    ? Math.max(20, (viewportW / contentWidth) * viewportW)
    : 0;
  const barLeft =
    canScrollH && contentWidth - viewportW > 0
      ? (scrollLeft / (contentWidth - viewportW)) * (viewportW - barWidth)
      : 0;

  // ----- Socket.io connection -----
  useEffect(() => {
    const socket = io('/?XTransformPort=3003', {
      transports: ['websocket', 'polling'],
      reconnection: true,
    });
    socketRef.current = socket;

    socket.on('layout', (data: Layout) => {
      setLoadingTabId(null);
      const targetTabId = navTabIdRef.current;
      if (targetTabId == null) return;
      setTabs((prev) =>
        prev.map((t) => {
          if (t.id !== targetTabId) return t;
          const title = data.title || t.title;
          return { ...t, layout: data, title };
        }),
      );
      if (scrollRef.current) {
        scrollRef.current.scrollTop = 0;
        // CHANGED WITH AI: also reset horizontal scroll on new navigation,
        // so the new page starts at the left edge (not whatever scrollLeft
        // the previous page had).
        scrollRef.current.scrollLeft = 0;
      }
      setScrollTop(0);
      setScrollLeft(0);
    });

    socket.on('bookmarks', (data: string[]) => {
      setBookmarks(Array.isArray(data) ? data : []);
    });

    socket.emit('getBookmarks');

    return () => {
      socket.disconnect();
    };
  }, []);

  // ----- Cursor blink (every 500ms) -----
  useEffect(() => {
    const interval = setInterval(() => setCursorVisible((v) => !v), 500);
    return () => clearInterval(interval);
  }, []);

  // ----- Measure URL bar text width for the custom caret -----
  useEffect(() => {
    if (urlMeasureRef.current) {
      setTextWidth(urlMeasureRef.current.offsetWidth);
    }
  }, [urlInput]);

  // ----- Custom scrollbar drag (window-level listeners) -----
  // CHANGED WITH AI: now handles BOTH vertical and horizontal scrollbar drags.
  // The dragStateRef holds an `axis` field ('v' or 'h') so we know which
  // scroll dimension to update. The math is the same in both cases — just
  // swap Y/X and viewportH/viewportW.
  useEffect(() => {
    const onMove = (e: MouseEvent) => {
      const ds = dragStateRef.current;
      if (!ds || !scrollRef.current) return;

      if (ds.axis === 'v') {
        const vph = viewportHRef.current;
        const ch = contentHeightRef.current;
        if (ch <= vph) return;
        const bH = Math.max(20, (vph / ch) * vph);
        const dy = e.clientY - ds.startY;
        const scrollRange = ch - vph;
        const barRange = vph - bH;
        if (barRange <= 0) return;
        const newScroll = ds.startScroll + (dy / barRange) * scrollRange;
        scrollRef.current.scrollTop = Math.max(0, Math.min(scrollRange, newScroll));
      } else {
        // axis === 'h'
        const vpw = viewportWRef.current;
        const cw = contentWidthRef.current;
        if (cw <= vpw) return;
        const bW = Math.max(20, (vpw / cw) * vpw);
        const dx = e.clientX - ds.startX;
        const scrollRange = cw - vpw;
        const barRange = vpw - bW;
        if (barRange <= 0) return;
        const newScroll = ds.startScroll + (dx / barRange) * scrollRange;
        scrollRef.current.scrollLeft = Math.max(0, Math.min(scrollRange, newScroll));
      }
    };
    const onUp = () => {
      dragStateRef.current = null;
    };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    return () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
  }, []);

  // ----- Navigate: emit to engine, update tab history -----
  function navigate(
    url: string,
    opts?: { addToHistory?: boolean; tabId?: number; historyPos?: number },
  ) {
    const socket = socketRef.current;
    if (!socket) return;
    const targetTabId = opts?.tabId ?? activeTabData?.id;
    if (targetTabId == null) return;
    navTabIdRef.current = targetTabId;
    setLoadingTabId(targetTabId);

    const addToHistory = opts?.addToHistory ?? true;
    setTabs((prev) =>
      prev.map((t) => {
        if (t.id !== targetTabId) return t;
        let history = t.history;
        let newHistoryPos = t.historyPos;
        if (addToHistory) {
          if (t.history[t.historyPos] !== url) {
            history = [...t.history.slice(0, t.historyPos + 1), url];
            newHistoryPos = history.length - 1;
          }
        } else if (opts?.historyPos != null) {
          newHistoryPos = opts.historyPos;
        }
        return { ...t, url, history, historyPos: newHistoryPos };
      }),
    );

    if (activeTabData?.id === targetTabId) {
      setUrlInput(url === 'home' ? '' : url);
    }

    socket.emit('navigate', { url });
  }

  // ----- Load the home page on first mount -----
  useEffect(() => {
    // One-time initial navigation; setState here is intentional.
    // eslint-disable-next-line react-hooks/set-state-in-effect
    navigate('home', { addToHistory: true });
  }, []);

  // ----- Event handlers -----
  function handleUrlSubmit() {
    const url = urlInput.trim();
    if (!url) {
      navigate('home', { addToHistory: true });
      return;
    }
    navigate(url, { addToHistory: true });
  }

  function handleUrlKeyDown(e: React.KeyboardEvent<HTMLInputElement>) {
    if (e.key === 'Enter') {
      e.preventDefault();
      handleUrlSubmit();
    }
  }

  function handleBack() {
    if (!activeTabData || activeTabData.historyPos <= 0) return;
    const newPos = activeTabData.historyPos - 1;
    const url = activeTabData.history[newPos];
    navigate(url, { addToHistory: false, historyPos: newPos });
  }

  function handleForward() {
    if (
      !activeTabData ||
      activeTabData.historyPos >= activeTabData.history.length - 1
    )
      return;
    const newPos = activeTabData.historyPos + 1;
    const url = activeTabData.history[newPos];
    navigate(url, { addToHistory: false, historyPos: newPos });
  }

  function handleReload() {
    if (!activeTabData) return;
    navigate(activeTabData.url, { addToHistory: false });
  }

  function handleHome() {
    navigate('home', { addToHistory: true });
  }

  function handleStar() {
    if (!currentUrl || currentUrl === 'home') return;
    const add = !isBookmarked;
    socketRef.current?.emit('bookmark', { url: currentUrl, add });
  }

  function handlePrint() {
    window.print();
  }

  function handleLinkClick(href: string, e: React.MouseEvent) {
    e.preventDefault();
    e.stopPropagation();
    // CHANGED WITH AI: skip links that the engine normalized to "" — these are
    // javascript: URLs, pure-fragment links (#foo), and other non-navigable
    // hrefs. Without this guard, clicking a Bing menu button (href=
    // "javascript:void(0)") would navigate to "" and the engine would treat
    // empty input as a "home" load, kicking the user back to the start page.
    if (!href || href.trim() === '') {
      return;
    }
    navigate(href, { addToHistory: true });
  }

  function handleNewTab() {
    const newTab = makeTab('New Tab', 'home');
    setTabs((prev) => [...prev, newTab]);
    setActiveTab(tabs.length);
    // CHANGED WITH AI: explicitly clear the URL bar for the new tab. navigate()
    // checks activeTabData?.id === targetTabId, but activeTabData is stale here
    // (React batches the setTabs/setActiveTab updates), so it would keep the
    // previous tab's URL in the bar.
    setUrlInput('');
    navigate('home', { addToHistory: true, tabId: newTab.id });
  }

  function handleCloseTab(tabId: number, e: React.MouseEvent) {
    e.stopPropagation();
    const idx = tabs.findIndex((t) => t.id === tabId);
    if (idx === -1) return;
    tabScrollRef.current.delete(tabId);
    const newTabs = tabs.filter((t) => t.id !== tabId);
    if (newTabs.length === 0) {
      const newTab = makeTab('New Tab', 'home');
      setTabs([newTab]);
      setActiveTab(0);
      // CHANGED WITH AI: clear URL bar for the freshly created tab (same stale-state bug as handleNewTab).
      setUrlInput('');
      navigate('home', { addToHistory: true, tabId: newTab.id });
      return;
    }
    setTabs(newTabs);
    if (activeTab === idx) {
      const newActive = Math.min(idx, newTabs.length - 1);
      setActiveTab(newActive);
      const t = newTabs[newActive];
      setUrlInput(t.url === 'home' ? '' : t.url);
      requestAnimationFrame(() => {
        if (scrollRef.current) {
          scrollRef.current.scrollTop = tabScrollRef.current.get(t.id) ?? 0;
          setScrollTop(scrollRef.current.scrollTop);
        }
      });
    } else if (activeTab > idx) {
      setActiveTab(activeTab - 1);
    }
  }

  function handleSelectTab(idx: number) {
    if (idx === activeTab) return;
    if (activeTabData && scrollRef.current) {
      tabScrollRef.current.set(activeTabData.id, scrollRef.current.scrollTop);
    }
    setActiveTab(idx);
    const t = tabs[idx];
    setUrlInput(t.url === 'home' ? '' : t.url);
    requestAnimationFrame(() => {
      if (scrollRef.current) {
        scrollRef.current.scrollTop = tabScrollRef.current.get(t.id) ?? 0;
        setScrollTop(scrollRef.current.scrollTop);
      }
    });
  }

  function handleScroll() {
    if (scrollRef.current) {
      setScrollTop(scrollRef.current.scrollTop);
      setScrollLeft(scrollRef.current.scrollLeft);
      if (activeTabData) {
        tabScrollRef.current.set(activeTabData.id, scrollRef.current.scrollTop);
      }
    }
  }

  // CHANGED WITH AI: renamed to make it clear this is the VERTICAL scrollbar
  // thumb drag handler. Now sets `axis: 'v'` on the dragState.
  function handleScrollbarMouseDown(e: React.MouseEvent) {
    e.stopPropagation();
    e.preventDefault();
    if (!canScroll) return;
    dragStateRef.current = {
      axis: 'v',
      startY: e.clientY,
      startScroll: scrollRef.current?.scrollTop ?? 0,
    };
  }

  function handleTrackMouseDown(e: React.MouseEvent) {
    if (!scrollRef.current || !canScroll) return;
    const trackRect = (e.currentTarget as HTMLDivElement).getBoundingClientRect();
    const clickY = e.clientY - trackRect.top;
    const targetBarTop = clickY - barHeight / 2;
    const barRange = viewportH - barHeight;
    const scrollRange = contentHeight - viewportH;
    if (barRange <= 0) return;
    const newScroll = (targetBarTop / barRange) * scrollRange;
    scrollRef.current.scrollTop = Math.max(0, Math.min(scrollRange, newScroll));
  }

  // CHANGED WITH AI: horizontal scrollbar thumb drag handler — mirror of
  // handleScrollbarMouseDown but for the X axis.
  function handleHScrollbarMouseDown(e: React.MouseEvent) {
    e.stopPropagation();
    e.preventDefault();
    if (!canScrollH) return;
    dragStateRef.current = {
      axis: 'h',
      startX: e.clientX,
      startScroll: scrollRef.current?.scrollLeft ?? 0,
    };
  }

  // CHANGED WITH AI: horizontal scrollbar track click — mirror of
  // handleTrackMouseDown but for the X axis. Clicking the track jumps the
  // thumb to that position.
  function handleHTrackMouseDown(e: React.MouseEvent) {
    if (!scrollRef.current || !canScrollH) return;
    const trackRect = (e.currentTarget as HTMLDivElement).getBoundingClientRect();
    const clickX = e.clientX - trackRect.left;
    const targetBarLeft = clickX - barWidth / 2;
    const barRange = viewportW - barWidth;
    const scrollRange = contentWidth - viewportW;
    if (barRange <= 0) return;
    const newScroll = (targetBarLeft / barRange) * scrollRange;
    scrollRef.current.scrollLeft = Math.max(0, Math.min(scrollRange, newScroll));
  }

  function renderLayoutItem(item: LayoutItem, i: number) {
    if (item.type === 'image') {
      const imgStyle: React.CSSProperties = {
        position: 'absolute',
        left: item.x,
        top: item.y,
        width: item.width,
        height: item.height,
        objectFit: 'contain',
        imageRendering: 'pixelated',
        cursor: item.href ? 'pointer' : 'default',
      };
      return (
        <img
          key={i}
          src={item.src}
          alt=""
          style={imgStyle}
          draggable={false}
          onError={(e) => {
            // CHANGED WITH AI: hide broken images entirely instead of showing
            // the default broken-image placeholder.
            e.currentTarget.style.display = 'none';
          }}
          onClick={
            item.href ? (e) => handleLinkClick(item.href as string, e) : undefined
          }
        />
      );
    }
    const textStyle: React.CSSProperties = {
      position: 'absolute',
      left: item.x,
      top: item.y,
      fontSize: item.fontSize,
      color: item.color || '#000',
      fontFamily: PX_FONT,
      whiteSpace: 'nowrap',
      lineHeight: 1,
      background: item.bgColor || 'transparent',
    };
    if (item.href) {
      textStyle.textDecoration = 'underline';
      textStyle.cursor = 'pointer';
    }
    return (
      <span
        key={i}
        style={textStyle}
        onClick={
          item.href ? (e) => handleLinkClick(item.href as string, e) : undefined
        }
      >
        {item.text}
      </span>
    );
  }

  const backDisabled = !activeTabData || activeTabData.historyPos <= 0;
  const forwardDisabled =
    !activeTabData || activeTabData.historyPos >= activeTabData.history.length - 1;
  const showLoading = loadingTabId != null && loadingTabId === activeTabData?.id;

  // CHANGED WITH AI: added hover/active highlight to match the visual response
  // expected from nav buttons. The Windows SDL version uses #c8c8c8 for enabled
  // and #ebebeb for disabled; we darken to #b4b4b4 on hover for enabled buttons.
  function btnStyle(disabled?: boolean, bg?: string, btnName?: string): React.CSSProperties {
    let background = bg ?? (disabled ? '#ebebeb' : '#c8c8c8');
    if (!disabled && btnName && hoveredBtn === btnName) {
      background = bg ?? '#b4b4b4';
    }
    return {
      width: 30,
      height: 30,
      background,
      border: 'none',
      padding: 0,
      margin: 0,
      cursor: disabled ? 'default' : 'pointer',
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center',
      flexShrink: 0,
      fontFamily: PX_FONT,
      color: '#000',
      fontSize: 28,
      lineHeight: 1,
      boxSizing: 'border-box',
      outline: 'none',
      opacity: 1,
    };
  }

  const iconSpanStyle: React.CSSProperties = {
    fontSize: 28,
    color: '#000',
    fontFamily: PX_FONT,
    lineHeight: 1,
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
  };

  return (
    <div
      style={{
        height: '100vh',
        display: 'flex',
        flexDirection: 'column',
        overflow: 'hidden',
        fontFamily: PX_FONT,
        background: '#f5f5f5',
        WebkitFontSmoothing: 'none',
      }}
    >
      {/* ===== Tab bar ===== */}
      <div
        style={{
          height: TAB_BAR_H,
          background: '#d2d2d2',
          display: 'flex',
          alignItems: 'center',
          padding: `0 ${TAB_GAP}px`,
          gap: TAB_GAP,
          flexShrink: 0,
          WebkitUserSelect: 'none',
          userSelect: 'none',
        }}
      >
        {tabs.map((tab, idx) => (
          <div
            key={tab.id}
            onClick={() => handleSelectTab(idx)}
            style={{
              width: TAB_W,
              height: TAB_BAR_H,
              background: idx === activeTab ? '#f5f5f5' : '#bebebe',
              display: 'flex',
              alignItems: 'center',
              position: 'relative',
              cursor: 'pointer',
              flexShrink: 0,
              overflow: 'hidden',
            }}
            title={tab.title}
          >
            <span
              style={{
                fontSize: 13,
                color: '#000',
                paddingLeft: 8,
                paddingRight: 28,
                fontFamily: PX_FONT,
                whiteSpace: 'nowrap',
                overflow: 'hidden',
                textOverflow: 'ellipsis',
                flex: 1,
                lineHeight: 1,
              }}
            >
              {truncateTitle(tab.title)}
            </span>
            {tabs.length > 1 && (
              <span
                onClick={(e) => handleCloseTab(tab.id, e)}
                style={{
                  position: 'absolute',
                  right: 6,
                  top: '50%',
                  transform: 'translateY(-50%)',
                  fontSize: 13,
                  color: '#000',
                  cursor: 'pointer',
                  fontFamily: PX_FONT,
                  lineHeight: 1,
                  padding: '2px 4px',
                }}
                title="Close tab"
              >
                x
              </span>
            )}
          </div>
        ))}
        <div
          onClick={handleNewTab}
          style={{
            width: 26,
            height: 26,
            background: '#b4b4b4',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            cursor: 'pointer',
            flexShrink: 0,
            marginLeft: 2,
            fontSize: 13,
            color: '#000',
            fontFamily: PX_FONT,
            lineHeight: 1,
            WebkitUserSelect: 'none',
            userSelect: 'none',
          }}
          title="New tab"
        >
          +
        </div>
      </div>

      {/* ===== Nav bar ===== */}
      <div
        style={{
          height: NAV_BAR_H,
          background: '#f5f5f5',
          display: 'flex',
          alignItems: 'center',
          padding: '0 15px',
          gap: 15,
          flexShrink: 0,
        }}
      >
        <button
          onClick={handleBack}
          disabled={backDisabled}
          style={btnStyle(backDisabled, undefined, 'back')}
          onMouseEnter={() => setHoveredBtn('back')}
          onMouseLeave={() => setHoveredBtn(null)}
          title="Back"
          aria-label="Back"
        >
          {/* CHANGED WITH AI: back arrow — shifted right 7.4px to center the
              glyph (advance width has 14.7px right bearing, so flex centering
              leaves the glyph 7.4px left of button center). */}
          <span style={{ ...iconSpanStyle, transform: 'translateX(7.4px)' }}>ђ</span>
        </button>
        <button
          onClick={handleForward}
          disabled={forwardDisabled}
          style={btnStyle(forwardDisabled, undefined, 'forward')}
          onMouseEnter={() => setHoveredBtn('forward')}
          onMouseLeave={() => setHoveredBtn(null)}
          title="Forward"
          aria-label="Forward"
        >
          {/* CHANGED WITH AI: forward arrow — mirrored + shifted left 7.4px to
              center the glyph (after mirroring, the glyph sits 7.4px right of
              center, so translateX(-7.4px) brings it back). */}
          <span
            style={{
              ...iconSpanStyle,
              transform: 'translateX(-7.4px) scaleX(-1)',
            }}
          >
            ђ
          </span>
        </button>
        <button
          onClick={handleReload}
          style={btnStyle(false, undefined, 'reload')}
          onMouseEnter={() => setHoveredBtn('reload')}
          onMouseLeave={() => setHoveredBtn(null)}
          title="Reload"
          aria-label="Reload"
        >
          {/* CHANGED WITH AI: reload icon — shifted right 1.1px to center the
              glyph, and moved up 2px (user requested "up a tiny bit"). */}
          <span style={{ ...iconSpanStyle, transform: 'translate(1.1px, -2px)' }}>њ</span>
        </button>
        <button
          onClick={handleHome}
          style={btnStyle(false, undefined, 'home')}
          onMouseEnter={() => setHoveredBtn('home')}
          onMouseLeave={() => setHoveredBtn(null)}
          title="Home"
          aria-label="Home"
        >
          {/* CHANGED WITH AI: home icon — shifted right 2.1px to center the
              glyph (advance width has 4.2px right bearing). */}
          <span style={{ ...iconSpanStyle, transform: 'translateX(2.1px)' }}>љ</span>
        </button>

        {/* URL / address bar */}
        <div
          style={{
            flex: 1,
            height: 30,
            background: '#f0f0f0',
            position: 'relative',
            display: 'flex',
            alignItems: 'center',
            minWidth: 0,
          }}
        >
          <span
            ref={urlMeasureRef}
            style={{
              position: 'absolute',
              visibility: 'hidden',
              whiteSpace: 'pre',
              fontSize: 17,
              fontFamily: PX_FONT,
              left: 8,
              top: 0,
              pointerEvents: 'none',
              lineHeight: 1,
            }}
          >
            {urlInput}
          </span>
          <input
            type="text"
            value={urlInput}
            onChange={(e) => setUrlInput(e.target.value)}
            onKeyDown={handleUrlKeyDown}
            onFocus={() => {
              setUrlFocused(true);
              // CHANGED WITH AI: reset the blink phase so the typing caret shows
              // IMMEDIATELY when the bar is clicked. The blink interval runs
              // continuously (every 500ms) even when the bar isn't focused, so
              // without this reset, clicking during the "off" phase would show
              // no caret for up to 500ms — which felt like the cursor wasn't
              // appearing at all.
              setCursorVisible(true);
            }}
            onBlur={() => setUrlFocused(false)}
            placeholder={urlFocused ? '' : 'ы Enter a url...'}
            className="url-input"
            spellCheck={false}
            autoComplete="off"
            autoCapitalize="off"
            autoCorrect="off"
            style={{
              width: '100%',
              height: '100%',
              background: 'transparent',
              border: 'none',
              outline: 'none',
              fontSize: 17,
              color: '#000',
              fontFamily: PX_FONT,
              paddingLeft: 8,
              paddingRight: 8,
              caretColor: 'transparent',
              lineHeight: 1,
            }}
            aria-label="URL"
          />
          {urlFocused && cursorVisible && (
            <div
              style={{
                position: 'absolute',
                // CHANGED WITH AI: position the cursor right after the measured
                // text width (or at the start of the input when empty, since
                // textWidth is 0 then). Removed the old `urlInput &&` gate so
                // the cursor also shows when the bar is empty and clicked —
                // previously the cursor only appeared when the bar already had
                // text, which made clicking an empty bar feel broken.
                left: 8 + textWidth + 1,
                top: 5,
                width: 1,
                height: 18,
                background: '#000',
                pointerEvents: 'none',
              }}
            />
          )}
        </div>

        <button
          onClick={handleStar}
          style={btnStyle(false, isBookmarked ? '#ffff00' : '#c8c8c8', 'star')}
          onMouseEnter={() => setHoveredBtn('star')}
          onMouseLeave={() => setHoveredBtn(null)}
          title={isBookmarked ? 'Remove bookmark' : 'Add bookmark'}
          aria-label="Bookmark"
        >
          {/* star icon — already perfectly centered (centerOffset=0), no shift needed */}
          <span style={iconSpanStyle}>ж</span>
        </button>
        <button
          onClick={handlePrint}
          style={btnStyle(false, undefined, 'print')}
          onMouseEnter={() => setHoveredBtn('print')}
          onMouseLeave={() => setHoveredBtn(null)}
          title="Print"
          aria-label="Print"
        >
          {/* CHANGED WITH AI: printer icon — shifted right 4.2px to center the
              glyph (advance width has 8.4px right bearing). */}
          <span style={{ ...iconSpanStyle, transform: 'translateX(4.2px)' }}>ξ</span>
        </button>
      </div>

      {/* ===== Content area ===== */}
      <div
        style={{
          flex: 1,
          position: 'relative',
          background: currentBg,
          overflow: 'hidden',
        }}
      >
        <div
          ref={scrollRef}
          onScroll={handleScroll}
          className="hide-native-scroll"
          style={{
            position: 'absolute',
            // CHANGED WITH AI: reserve SCROLL_W at the bottom for the horizontal
            // scrollbar when it's visible (mirrors how the vertical scrollbar
            // reserves SCROLL_W on the right).
            inset: canScrollH ? `0 ${canScroll ? SCROLL_W : 0}px ${SCROLL_W}px 0` : `0 ${canScroll ? SCROLL_W : 0}px 0 0`,
            overflowY: 'auto',
            overflowX: 'auto',
          }}
        >
          <div
            style={{
              position: 'relative',
              // CHANGED WITH AI: use the computed contentWidth (which is the
              // widest layout item) so the inner content can be wider than the
              // viewport, enabling horizontal scrolling. Was `width: '100%'`
              // which clamped everything to the viewport width.
              width: contentWidth,
              height: contentHeight,
            }}
          >
            {currentLayout?.items?.map((item, i) => renderLayoutItem(item, i))}
          </div>
        </div>

        {/* Loading overlay (white screen + "Loading..." h2, like reload.html) */}
        {showLoading && (
          <div
            style={{
              position: 'absolute',
              inset: 0,
              background: '#ffffff',
              display: 'flex',
              alignItems: 'flex-start',
              justifyContent: 'flex-start',
              padding: '20px',
              zIndex: 10,
              pointerEvents: 'none',
            }}
          >
            <h2
              style={{
                fontSize: 24,
                color: '#000',
                fontFamily: PX_FONT,
                margin: 0,
                lineHeight: 1,
              }}
            >
              Loading...
            </h2>
          </div>
        )}

        {/* Error message */}
        {currentLayout?.error && !showLoading && (
          <div
            style={{
              position: 'absolute',
              top: 20,
              left: 20,
              right: 20,
              color: '#000',
              fontFamily: PX_FONT,
              fontSize: 16,
              zIndex: 8,
              lineHeight: 1.4,
              whiteSpace: 'pre-wrap',
            }}
          >
            Error: {currentLayout.error}
          </div>
        )}

        {/* Custom Windows-style vertical scrollbar */}
        {/* CHANGED WITH AI: shortened by SCROLL_W at the bottom when the
            horizontal scrollbar is also visible, so the two don't overlap
            (a separate corner box fills the bottom-right gap). */}
        {canScroll && (
          <div
            onMouseDown={handleTrackMouseDown}
            style={{
              position: 'absolute',
              right: 0,
              top: 0,
              width: SCROLL_W,
              height: canScrollH ? viewportH - SCROLL_W : viewportH,
              background: '#e0e0e0',
              zIndex: 5,
            }}
          >
            <div
              onMouseDown={handleScrollbarMouseDown}
              style={{
                position: 'absolute',
                left: 0,
                top: barTop,
                width: SCROLL_W,
                height: barHeight,
                background: '#c0c0c0',
                cursor: 'grab',
                boxShadow:
                  'inset 1px 1px 0 #ffffff, inset -1px -1px 0 #646464',
              }}
            />
          </div>
        )}

        {/* CHANGED WITH AI: Custom Windows-style HORIZONTAL scrollbar.
            Mirrors the vertical one's styling (same colors, same inset bevel,
            same SCROLL_W thickness) but rotated 90°. Shows up at the bottom
            of the content area when the widest layout item exceeds the
            viewport width (e.g. wide Bing search result text). */}
        {canScrollH && (
          <div
            onMouseDown={handleHTrackMouseDown}
            style={{
              position: 'absolute',
              left: 0,
              bottom: 0,
              // CHANGED WITH AI: when the vertical scrollbar is also visible,
              // the H-scrollbar stops SCROLL_W short of the right edge so the
              // two don't overlap (the corner box fills the bottom-right gap).
              width: canScroll ? viewportW : viewportW,
              height: SCROLL_W,
              background: '#e0e0e0',
              zIndex: 5,
            }}
          >
            <div
              onMouseDown={handleHScrollbarMouseDown}
              style={{
                position: 'absolute',
                left: barLeft,
                top: 0,
                width: barWidth,
                height: SCROLL_W,
                background: '#c0c0c0',
                cursor: 'grab',
                boxShadow:
                  'inset 1px 1px 0 #ffffff, inset -1px -1px 0 #646464',
              }}
            />
          </div>
        )}

        {/* CHANGED WITH AI: corner box at the bottom-right where the two
            scrollbars meet. Without this, you'd see the page background
            through the gap. Filled with the same #e0e0e0 as the scrollbar
            tracks so it looks like one continuous frame. */}
        {canScroll && canScrollH && (
          <div
            style={{
              position: 'absolute',
              right: 0,
              bottom: 0,
              width: SCROLL_W,
              height: SCROLL_W,
              background: '#e0e0e0',
              zIndex: 5,
            }}
          />
        )}
      </div>
    </div>
  );
}
