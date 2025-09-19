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



// https://developer.mozilla.org/en-US/docs/Web/Progressive_web_apps/Manifest to store as webapp on phone
