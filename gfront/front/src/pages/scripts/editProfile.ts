import { initPage } from "../components/initPage";
import { createBut, createDiv, createForm, createImage, createInp, createLab, createListItem, createMain, createSelect, createSpan, createUl, truncateText } from "./cssTools";
import type { Profile } from "./profile";

initPage("edit", () => {
  const editContainer = document.getElementById("edit-profile-container");
  if (editContainer) {

	/// Testing!!
	
	
	const bob: Profile = 
	{
		id: "1234",
		username: "bobobo",
		email: "bob@bob.com",
		first_name: "Bob",
		last_name: "man",
		profpic: "",
		bio: "amazing spiderman",
		hashtags: [],
	};

	/////////////

	editContainer.appendChild(renderEditForm(bob));

	const form = document.getElementById("edit-form") as HTMLFormElement;
	if (form) {
	  form.addEventListener("submit", (e) => {
		e.preventDefault();
		const data = Object.fromEntries(new FormData(form));
		console.log("Edit attempt:", data);
	  });
	}
  }
});



function renderEditForm(user:Profile)
{

	const main = createMain("flex flex-grow items-center justify-center p-2 bg-emerald-100");
	const main_div = createDiv("w-full max-w-xs");

	
	//
	const avatarDiv = createDiv("mx-auto w-32 h-32 relative border-2 box-blue overflow-hidden mt-2");
	const avatarImg = createImage(user?.profpic || "/vite.svg", "profile picture", "object-cover w-full h-full");

	//
	const formsContainer = createDiv("px-6 py-4");

	//
	const imgForm = createForm("edit-img-form", "space-x-2");
	imgForm.enctype = "multipart/form-data";
	
	const fileInput = createInp("","file","image", "image-upload", "hidden");
	fileInput.type = "file";
	fileInput.accept = "image/*";

	const label = document.createElement("label");
	label.htmlFor = "image-upload";
	label.textContent = "Choose Image";
	label.className = "cursor-pointer btn-contrast hover-underline-animation px-4 py-2 inline-block";

	//
	const uploadError_wrapper = createDiv("mb-1 h-4");
	const uploadError = createSpan("", "upload-error", "text-red-500 text-xs italic hidden mb-4 animate-fade-in");

	const uploadBtn = createBut("Upload", "upload-image", "btn-contrast hover-underline-animation px-4 py-2 mt-4");
	uploadBtn.setAttribute("type", "submit");

	//

	const userForm = createForm("userForm", "mt-4");
	//


	const genderLabel = createLab(`Current gender: ${user?.gender}`, "gender", " text-blue font-bold mb-2");
	const genderSelect = createSelect("gender","box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none mb-4",
		[
			{value: "0", text: ""},
			{value: "1", text: "Not specified"},
			{value: "2", text: "Non-binary"},
			{value: "3", text: "Female"},
			{value: "4", text: "Male"},
		]
	);
	(genderSelect.querySelector("option[value='0']") as HTMLOptionElement).selected = true;

	//

	const orientLabel = createLab(`Sexual orientation: ${user?.orientation}`, "orient", " text-blue font-bold mb-2");
	const orientSelect = createSelect("orient","box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none mb-4",
		[
			{value: "0", text: ""},
			{value: "1", text: "Not specified"},
			{value: "2", text: "Bisexual"},
			{value: "3", text: "Women"},
			{value: "4", text: "Men"},
		]
	);
	(orientSelect.querySelector("option[value='0']") as HTMLOptionElement).selected = true;


	//
	const ageLabel = createLab(`Current birthday: ${user?.age}`, "age", " text-blue font-bold mb-2");
	const ageInput = createInp("dd/mm/yyyy", "text", "age", "age", "box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none");
	
	const age_error_wrapper = createDiv("mb-1 h-4");
	const age_error = createSpan("", "age-error-check", "text-red-500 text-xs animate-fade-in italic hidden");

	//
	const tmp_name = truncateText(user?.first_name,13);
	const name_label = createLab(`Current first name: ${tmp_name}`, "name", "text-blue font-bold mb-2");
	name_label.title = user.first_name;
	const name_input = createInp("example name", "text", "name", "name", "box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none");
	
	
	const name_error_wrapper = createDiv("mb-1 h-4");
	const name_error = createSpan("", "name-error-check", "text-red-500 text-xs animate-fade-in italic hidden");

	

	//
	const tmp_last = truncateText(user?.last_name,13);
	const lastname_label = createLab(`Current last name: ${tmp_last}`, "lastname", "text-blue font-bold mb-2");
	lastname_label.title = user?.last_name;
	const lastname_input = createInp("example last name", "text", "lastname", "lastname", "box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none");
	
	
	const lastname_error_wrapper = createDiv("mb-1 h-4");
	const lastname_error = createSpan("", "lastname-error-check", "text-red-500 text-xs animate-fade-in italic hidden");

	
	//
	const tmp_user = truncateText(user?.username,13);
	const userLabel = createLab(`Current username: ${tmp_user}`, "new-user", "text-blue font-bold mb-2");
	const userInput = createInp("Enter new username", "text", "new-user", "username", "box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none");
	
	
	const userError_wrapper = createDiv("mb-1 h-4");
	const userError = createSpan("", "user-error-check", "text-red-500 text-xs italic hidden mb-2 animate-fade-in");
	

	//
	
	const tmp_email = truncateText(user?.email,13);
	const emailLabel = createLab(`Current email: ${tmp_email}`, "new-email", "text-blue font-bold mb-2");
	emailLabel.title = user?.email;
	const emailInput = createInp("Enter new email", "email", "new-email", "new-email", "box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none");

	
	const emailError_wrapper = createDiv("mb-1 h-4");
	const emailError = createSpan("", "email-error-check", "text-red-500 text-xs italic hidden mb-2 animate-fade-in");
	

	//
	
	const passLabel = createLab("New Password", "new-pass", "text-blue font-bold mb-2");
	const passInput = createInp("Enter new password", "password", "new-pass", "new-pass", "box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none");
	const passInput2 = createInp("Repeat new password", "password", "new-pass2", "new-pass2", "box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none mt-2");
	
	const passRulesDiv = createDiv("mt-3 text-sm text-blue w-full");
	passRulesDiv.textContent = "Password requirements:";
	
	const passRulesList = createUl("list-disc text-left list-outside pl-5 mt-1 text-xs");
	const rule1 = createListItem("At least 7 characters", "");
	const rule2 = createListItem("One letter", "");
	const rule3 = createListItem("One number", "");
	const rule4 = createListItem("One special character (!@#$%^&*)", "");
	
	const passError_wrapper = createDiv("mb-1 h-4");
	const passError = createSpan("", "pass-error-check", "text-red-500 text-xs italic hidden mb-2 animate-fade-in");



	const saveChangesBtn = createBut("Save changes", "save-changes-btn", "hover-underline-animation btn-contrast mt-2 w-full");
	saveChangesBtn.setAttribute("type", "submit");
	//
	
	main.appendChild(main_div);
	main_div.appendChild(avatarDiv);
	main_div.appendChild(formsContainer);
	avatarDiv.appendChild(avatarImg);
	
	//
	formsContainer.appendChild(imgForm);
	imgForm.appendChild(uploadError_wrapper);
	uploadError_wrapper.appendChild(uploadError);
	imgForm.appendChild(fileInput);
	imgForm.appendChild(label);
	imgForm.appendChild(uploadBtn);
	
	//
	
	userForm.appendChild(genderLabel);
	userForm.appendChild(genderSelect);
	//

	userForm.appendChild(orientLabel);
	userForm.appendChild(orientSelect);

	//
	userForm.appendChild(ageLabel);
	userForm.appendChild(ageInput);
	userForm.appendChild(age_error_wrapper);
	userForm.appendChild(age_error);

	//
	userForm.appendChild(name_label);
	userForm.appendChild(name_input);
	userForm.appendChild(name_error_wrapper);
	name_error_wrapper.appendChild(name_error);
	
	
	//
	
	userForm.appendChild(lastname_label);
	userForm.appendChild(lastname_input);
	userForm.appendChild(lastname_error_wrapper);
	lastname_error_wrapper.appendChild(lastname_error);
	
	
	//
	formsContainer.appendChild(userForm);

	userForm.appendChild(userLabel);
	userForm.appendChild(userInput);
	userForm.appendChild(userError_wrapper);
	userError_wrapper.appendChild(userError);
	//
	
	userForm.appendChild(emailLabel);
	userForm.appendChild(emailInput);
	userForm.appendChild(emailError_wrapper);
	emailError_wrapper.appendChild(emailError);
	
	
	//
	
	userForm.appendChild(passLabel);
	userForm.appendChild(passInput);
	userForm.appendChild(passInput2);
	
	userForm.appendChild(passRulesDiv);
	passRulesDiv.appendChild(passRulesList);
	passRulesList.appendChild(rule1);
	passRulesList.appendChild(rule2);
	passRulesList.appendChild(rule3);
	passRulesList.appendChild(rule4);
	userForm.appendChild(passError_wrapper);
	passError_wrapper.appendChild(passError);
	
	userForm.appendChild(saveChangesBtn);

	return main;
}


