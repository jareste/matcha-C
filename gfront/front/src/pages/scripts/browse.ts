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

// https://www.w3schools.com/howto/howto_css_searchbar.asp