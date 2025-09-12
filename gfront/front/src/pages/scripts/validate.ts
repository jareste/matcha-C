import { initPage } from "../components/initPage";
import { createMain } from "./cssTools";

initPage("validate", () => {
	const validateContainer = document.getElementById("validate-container");

	if (!validateContainer)
		return;
	validateContainer.appendChild(renderValidate());


});


function renderValidate()
{
	const main = createMain("");


	return main;
}