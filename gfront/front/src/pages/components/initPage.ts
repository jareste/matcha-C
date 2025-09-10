import { renderNavbar } from "./navbar";

export function initPage(activePage: string, callback?: () => void) {
  document.addEventListener("DOMContentLoaded", () => {
    const navbar = document.getElementById("navbar");
    if (navbar) navbar.innerHTML = renderNavbar(activePage);

    if (callback) callback();
  });
}