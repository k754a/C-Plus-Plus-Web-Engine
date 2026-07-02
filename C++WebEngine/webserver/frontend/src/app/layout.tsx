// CHANGED WITH AI: Minimal layout for the standalone C++Browse frontend.
// Stripped the Geist fonts and shadcn Toaster from the scaffold version —
// the browser UI uses only the PixelifySans pixel font (loaded in globals.css).
import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "C++Browse",
  description: "C++Browse — a C++ web browser engine",
};

export default function RootLayout({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  return (
    <html lang="en">
      <body>{children}</body>
    </html>
  );
}
