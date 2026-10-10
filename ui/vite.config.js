import { minify } from "html-minifier-terser";
import { defineConfig } from "vite";
import { viteSingleFile } from "vite-plugin-singlefile";

const robot = process.env.TINY_ENGINEER_URL || "http://tiny-engineer.local";
// Anchored so page routes such as /animations and /tests stay on the dev server.
const apiRoutes = [
  "^/auth([?]|$)",
  "^/health([?]|$)",
  "^/settings([/?]|$)",
  "^/anim([?]|$)",
  "^/play([/?]|$)",
  "^/test/",
  "^/setup/",
];

const minifyHtml = {
  name: "minify-html",
  apply: "build",
  enforce: "post",
  transformIndexHtml: (html) =>
    minify(html, { collapseWhitespace: true, removeComments: true, minifyCSS: true }),
};

export default defineConfig({
  // One self-contained page: the firmware embeds it gzipped and serves it as-is.
  plugins: [viteSingleFile(), minifyHtml],
  test: {
    environment: "jsdom",
  },
  server: {
    host: "127.0.0.1",
    proxy: Object.fromEntries(apiRoutes.map((route) => [route, robot])),
  },
});
