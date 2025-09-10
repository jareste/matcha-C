import { initPage } from "../components/initPage";

initPage("browse", () => {
  const browseContainer = document.getElementById("browse-container");
  if (browseContainer) {
	browseContainer.innerHTML = renderBrowseContent();
  }
});


function renderBrowseContent()
{
	// work with proper ts builders from cssTools
	return '';
}