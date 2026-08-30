// CHANGED WITH AI: just a small thing added, and allows for next.js to work with the structure (super simple)
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
