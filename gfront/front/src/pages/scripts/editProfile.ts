import { initPage } from "../components/initPage";
import { createBut, createDiv, createForm, createH2, createH3, createImage, createInp, createLab, createListItem, createMain, createP, createSelect, createSpan, createUl, truncateText } from "./cssTools";
import type { Profile } from "./profile";

initPage("edit", () => {
  const editContainer = document.getElementById("edit-profile-container");
  if (editContainer) {

	/// Testing!!
	
	
const testprofile: Profile = 
{
	"id": "550e8400-e29b-41d4-a716-446655440000",
	"username": "jane_doe42",
	"email": "jane.doe@example.com",
	"first_name": "Jane",
	"last_name": "Doe",
	"gender": "3",
	"age": "",
	"orientation": "2",
	"bio": "Adventure enthusiast and coffee lover. Always up for hiking trips and deep conversations!",
	"fame_rating": "4.7",
	"gps_lat": "40.7128",
	"gps_lon": "-74.0060",
	"location_optout": false,
	"last_online": "2024-01-15T14:30:00Z",
	"profpic": "https://wallpapercave.com/wp/wp1845166.jpg",//"https://images.unsplash.com/photo-1587778082149-bd5b1bf5d3fa?q=80&w=987&auto=format&fit=crop&ixlib=rb-4.1.0&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D",
	"images": ["https://images.unsplash.com/photo-1595871465907-19020bb76ad1?q=80&w=987&auto=format&fit=crop&ixlib=rb-4.1.0&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D","https://images.unsplash.com/photo-1587584867790-cf443d163e87?q=80&w=987&auto=format&fit=crop&ixlib=rb-4.1.0&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D","https://images.unsplash.com/photo-1593839686924-4b344fac3f8f?q=80&w=987&auto=format&fit=crop&ixlib=rb-4.1.0&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D"],
	"hashtags": ["#travel","#hiking","#coffee","#photography","#nature"]
}

	/////////////

	editContainer.appendChild(renderEditForm(testprofile));
	editContainer.appendChild(renderTestEdit(testprofile));

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



// Te idea is that this form will replace user profile when it clicks the edit button instead of having its own page.
// Need to transform all to input fields with a button on its side to modify each field
// What will happen if a field is invalid? how sould we act? discard everything? push the changes that are valid only?
function renderTestEdit(profile:Profile)
{
	// create the form :)
	const card = createDiv("container mx-auto rounded-2xl overflow-hidden shadow-lg bg-white border");


	// Need to investigate how to build the drag n drop feature properly for this field and that it shows properly.
	const imgWrapper = createDiv("overflow-hidden");
	const img = createImage(`${profile.profpic}`,`${profile.first_name} ${profile.last_name}`,"w-full h-full object-cover object-center");
	imgWrapper.appendChild(img);


	const info = createDiv("p-4");


	const header = createDiv("flex justify-start gap-2");

	// Need to investigate why it expands whenever it wants
	const name = createInp("", "text", "name", "name", "text-xl font-semibold min-w-[50px] max-w-[100px]");
	name.value = `${profile.first_name}` || "";
	name.readOnly = true;

	
	const last_name = createInp("", "text", "last_name", "last_name", "text-xl font-semibold min-w-[50px] max-w-[100px]");
	last_name.value = `${profile.last_name}` || "";
	last_name.readOnly = true;

	// I hate data input... depends on browser language to struct day month year...
	const age = createInp("", "date", "age", "age", "text-gray-500 text-lg");
	age.value = `${profile.age}` || "N/A";
	name.readOnly = true;
	
	header.appendChild(name);
	header.appendChild(last_name);
	header.appendChild(age);


	// const meta = createDiv("text-sm text-gray-600 mt-1"); Pending
	// meta.textContent = `• ⭐ ${profile.fame_rating}`;


	// const orientation = createP(`${profile.gender} • ${profile.orientation}`,"text-gray-600 text-sm mt-1");
	const orient_div = createDiv("text-gray-600 text-sm mt-1 flex");
	const genderSelect = createSelect("gender","",
		[
			{value: "0", text: ""},
			{value: "1", text: "Not specified"},
			{value: "2", text: "Non-binary"},
			{value: "3", text: "Female"},
			{value: "4", text: "Male"},
		]
	);
	genderSelect.value = profile.gender || "0";

	const orient_div_spl = createDiv("text-gray-600 text-sm ml-2 mr-2");
	orient_div_spl.textContent = "•";
	const orientSelect = createSelect("orient","",
		[
			{value: "0", text: ""},
			{value: "1", text: "Not specified"},
			{value: "2", text: "Bisexual"},
			{value: "3", text: "Women"},
			{value: "4", text: "Men"},
		]
	);
	orientSelect.value = profile.orientation || "0";


	orient_div.appendChild(genderSelect);
	orient_div.appendChild(orient_div_spl);
	orient_div.appendChild(orientSelect);


	// To read for proper input box expansion: https://css-tricks.com/auto-growing-inputs-textareas/ 
	// Need to specify max bio lenght
	const aboutWrapper = createDiv("mt-4");
	const aboutTitle = createH3("About me", "font-semibold text-gray-800");
	const aboutText = createInp("", "text", "bio", "bio", "text-gray-600 text-sm mt-1 leading-relaxed w-full")
	aboutText.value = `${profile.bio}` || "Explain others about yourself";
	aboutText.readOnly = true;
	
	aboutWrapper.appendChild(aboutTitle);
	aboutWrapper.appendChild(aboutText);



	// Idea for tags: https://tailwindcss.com/plus/ui-blocks/application-ui/forms/checkboxes
	// Other option: https://www.w3schools.com/howto/howto_css_searchbar.asp

	const tagsWrapper = createDiv("mt-4");
	const tagsTitle = createH3("Interests", "font-semibold text-gray-800");
	const tagsContainer = createDiv("flex flex-wrap gap-2 mt-2");
	profile.hashtags?.forEach((tag) => {
		tagsContainer.appendChild(
			createSpan(tag, "", "px-3 py-1 bg-gray-100 rounded-full text-sm text-gray-700")
		);
	});
	tagsWrapper.appendChild(tagsTitle);
	tagsWrapper.appendChild(tagsContainer);

	// const buttonsWrapper = createDiv("mt-4 mb-4 flex justify-evenly");
	// const like = createBut("♥", "like", "text-green-500 text-4xl");
	// const dislike = createBut("✖", "dislike", "text-red-500 text-4xl");
	

	info.appendChild(header);
	// info.appendChild(meta);
	info.appendChild(orient_div);
	info.appendChild(aboutWrapper);
	info.appendChild(tagsWrapper);

	// buttonsWrapper.appendChild(dislike);
	// buttonsWrapper.appendChild(like);

	card.appendChild(imgWrapper);
	card.appendChild(info);
	// card.appendChild(buttonsWrapper);

	[name, last_name, age, aboutText].forEach(inp => {
		inp.addEventListener("click", () => {
			inp.readOnly = false;
			inp.focus();
		});

		inp.addEventListener("blur", () => {
			inp.readOnly = true;
		});

		// inp.addEventListener("input", () => {
		// 	inp.style.width = "auto"; // testing!!
		// 	inp.style.width = Math.min(inp.scrollWidth + 8, 200) + "px";
		// });
	});
	return card;
}



//	from this, the idea is to only keep username email and password forms.
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





// Forms manipulation // might discard after some tests XD
/*

const label = document.createElement("label");
label.textContent = "Name:";
label.className = "block text-blue-600 font-bold mb-2";
container.appendChild(label);


const input = document.createElement("input");
input.type = "text";
input.value = user?.name || "";
input.readOnly = true;
input.className = "border rounded w-full py-2 px-3 text-gray-700 focus:outline-none";
container.appendChild(input);


const editButton = document.createElement("button");
editButton.textContent = "Edit";
editButton.className = "mt-2 bg-blue-500 text-white px-4 py-2 rounded hover:bg-blue-600";
container.appendChild(editButton);


editButton.addEventListener("click", () => {
  input.readOnly = false;
  input.focus();
});
*/

