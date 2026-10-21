import { defineConfig } from "vite";
import tailwindcss from '@tailwindcss/vite'

export default defineConfig({
  root: "src/pages",
  server:
  {
    host: "0.0.0.0",
    port: 3000,
  },
  plugins: [tailwindcss(),],
});
