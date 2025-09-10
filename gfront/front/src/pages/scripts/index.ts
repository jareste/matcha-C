import { initPage } from "../components/initPage";

initPage("home", () => {
  const homeContainer = document.getElementById("home-container");
  if (homeContainer) {
    homeContainer.innerHTML = renderHomeContent();
  }
});


function renderHomeContent()
{
	// work with proper ts builders from cssTools
	return '';
}