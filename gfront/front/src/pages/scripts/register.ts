import { initPage } from "../components/initPage";
import { createBtn, createDiv, createForm, createInp, createLab, createListItem, createMain, createSpan, createUl, showError } from "./tools";
import { checkValidEmail, checkValidPass, checkValidUser } from "./regex";

initPage("register", () => {
	const registerContainer = document.getElementById("register-container");
	
	if (!registerContainer)
		return;
	registerContainer.appendChild(renderRegisterForm());

	const form = document.getElementById("regForm") as HTMLFormElement;
	if (form) {
		setupRegisterForm(form);
	}
});

function renderRegisterForm()
{

	const main = createMain("flex flex-grow items-center justify-center p-2 bg-emerald-100");
	const main_div = createDiv("w-full max-w-xs");
	const form = createForm("regForm", "px-8 pt-6 pb-8 mb-4");
	form.setAttribute("novalidate", "");
	
	//
	const general_error = createDiv("text-red-500 text-xs italic animate-fade-in mb-2 hidden");
	general_error.setAttribute("id", "userfail");
	form.appendChild(general_error);
	
	//
	const email_div = createDiv("mb-4 mt-2");
	const email_label = createLab("Email", "email", "block text-blue font-bold mb-2");
	const email_input = createInp("example@email.com", "text", "email", "email", "box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none");
	email_input.setAttribute("required", "");
	
	const email_error_wrapper = createDiv("mb-1 h-4");
	const email_error = createSpan("", "email-error-check", "text-red-500 animate-fade-in text-xs italic hidden");
	//


	const user_div = createDiv("mb-4");
	const user_label = createLab("Username", "username", "block text-blue font-bold mb-2");
	const user_input = createInp("example username", "text", "username", "username", "box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none");
	user_input.setAttribute("required", "");
	
	const user_error_wrapper = createDiv("mb-1 h-4");
	const user_error = createSpan("", "user-error-check", "text-red-500 text-xs animate-fade-in italic hidden");

	//

	const name_div = createDiv("mb-4");
	const name_label = createLab("Name", "name", "block text-blue font-bold mb-2");
	const name_input = createInp("example name", "text", "name", "name", "box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none");
	name_input.setAttribute("required", "");
	
	const name_error_wrapper = createDiv("mb-1 h-4");
	const name_error = createSpan("", "name-error-check", "text-red-500 text-xs animate-fade-in italic hidden");

	//
	const lastname_div = createDiv("mb-4");
	const lastname_label = createLab("Last name", "lastname", "block text-blue font-bold mb-2");
	const lastname_input = createInp("example last name", "text", "lastname", "lastname", "box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none");
	lastname_input.setAttribute("required", "");
	
	const lastname_error_wrapper = createDiv("mb-1 h-4");
	const lastname_error = createSpan("", "lastname-error-check", "text-red-500 text-xs animate-fade-in italic hidden");

	//
	const pass_div = createDiv("");
	const pass_label = createLab("Password", "password", "block text-blue font-bold mb-2");
	const pass_input = createInp("", "password", "password", "password", "box-blue text-gray-500 shadow border rounded w-full py-2 px-3 leading-tight focus:outline-none");
	pass_input.setAttribute("required", "");
	

	const passrep_div = createDiv("mt-2");
	const passrep_input = createInp("repeat password", "password", "password2", "password2", "box-blue text-gray-500 shadow border rounded w-full py-2 px-3 leading-tight focus:outline-none");
	passrep_input.setAttribute("required", "");

	const pass_error_wrapper = createDiv("h-2");
	const pass_error = createSpan("", "pass-error-check", "text-red-500 animate-fade-in text-xs italic hidden");
	
	const passRulesDiv = createDiv("mt-3 text-sm text-blue ");
	passRulesDiv.textContent = "Password requirements:";
	
	const passRulesList = createUl("list-disc text-left list-outside pl-5 mt-1 text-xs");
	const rule1 = createListItem("At least 7 characters", "");
	const rule2 = createListItem("One letter", "");
	const rule3 = createListItem("One number", "");
	const rule4 = createListItem("One special character (!@#$%^&*)", "");
	
	//
	const butt_div = createDiv("flex items-center justify-between mt-6 mb-2");
	const register_button = createBtn("Register", "regBtnConfirm", "btn-contrast hover-underline-animation");
	register_button.setAttribute("type", "submit");
	
	//
	email_div.appendChild(email_label);
	email_div.appendChild(email_input);
	email_div.appendChild(email_error_wrapper);
	email_error_wrapper.appendChild(email_error);


	user_div.appendChild(user_label);
	user_div.appendChild(user_input);
	user_div.appendChild(user_error_wrapper);
	user_error_wrapper.appendChild(user_error);

	name_div.appendChild(name_label);
	name_div.appendChild(name_input);
	name_div.appendChild(name_error_wrapper);
	name_error_wrapper.appendChild(name_error);
	
	lastname_div.appendChild(lastname_label);
	lastname_div.appendChild(lastname_input);
	lastname_div.appendChild(lastname_error_wrapper);
	lastname_error_wrapper.appendChild(lastname_error);
	
	pass_div.appendChild(pass_label);
	pass_div.appendChild(pass_input);

	passrep_div.appendChild(passrep_input);

	passrep_div.appendChild(pass_error_wrapper);
	pass_error_wrapper.appendChild(pass_error);
	passrep_div.appendChild(passRulesDiv);
	passRulesDiv.appendChild(passRulesList);
	passRulesList.appendChild(rule1);
	passRulesList.appendChild(rule2);
	passRulesList.appendChild(rule3);
	passRulesList.appendChild(rule4);
	
	butt_div.appendChild(register_button);
	
	form.appendChild(email_div);
	form.appendChild(user_div);
	form.appendChild(name_div);
	form.appendChild(lastname_div);
	form.appendChild(pass_div);
	form.appendChild(passrep_div);
	form.appendChild(butt_div);
	
	main_div.appendChild(form);
	main.appendChild(main_div);

	return main;
}



// extra notes: submitter property of SubmitEvent tells you which button triggered the submit.

export function setupRegisterForm(regForm: HTMLFormElement) {

	regForm.addEventListener("submit", async (e) => {
		e.preventDefault();
		
		const formData = new FormData(regForm);
		const email = (formData.get("email") as string)?.trim();
		const username = (formData.get("username") as string)?.trim();
		const first_name = (formData.get("name") as string)?.trim();
		const last_name = (formData.get("lastname") as string)?.trim();
		const password = (formData.get("password") as string) ?? "";
		const password2 = (formData.get("password2") as string) ?? "";
		
		document.getElementById("userfail")?.classList.add("hidden");
		document.getElementById("email-error-check")?.classList.add("hidden");
		document.getElementById("user-error-check")?.classList.add("hidden");
		document.getElementById("name-error-check")?.classList.add("hidden");
		document.getElementById("lastname-error-check")?.classList.add("hidden");
		document.getElementById("pass-error-check")?.classList.add("hidden");
		
		let hasError = false;
		
		//
		if (!email) {
			showError("email-error-check", "Email is required");
			hasError = true;
		} else if (!checkValidEmail(email)) {
			showError("email-error-check", "Please enter a valid email address");
			hasError = true;
		}
		
		//
		if (!username) {
			showError("user-error-check", "Username is required");
			hasError = true;
		} else if (!checkValidUser(username)) {
			showError("user-error-check", "Username must be 2-16 characters long");
			hasError = true;
		}

		if (!first_name) {
			showError("name-error-check", "Name is required");
			hasError = true;
		}

		if (!last_name) {
			showError("lastname-error-check", "Last name is required");
			hasError = true;
		}
		
		//
		if (!password || !password2) {
			showError("pass-error-check", "Password is required in both fields");
			hasError = true;
		} else if (!checkValidPass(password, password2)) {
			showError("pass-error-check", "Passwords must meet the requirements and match each other");
			hasError = true;
		}

		
		if (hasError) return;
		
		try {
			const res = await fetch(`http://localhost:8080/api/register`, { //pending proper url or whatever will be the path
				method: "POST",
				body: JSON.stringify({
					username,
					email,
					password,
					first_name,
					last_name,
				}),
				headers: {
					"Content-Type": "application/json",
				},
				credentials: 'include',
			});
			
			if (!res.ok) {
				showError("userfail", "Email or Username already in use");
				throw new Error(`Request failed: ${res.status}`);
			}
			const data = await res.json();
			console.log("Success:", data);

			//localStorage.setItem( token?? , data.token??); Something similar?
			
			// store info init socket etc??

			//nav to next setting page to fill extra info for profie
			
		} catch (err) {
			console.error("Error during registration:", err);
			showError("userfail", "Username or email is already in use");
		}
	});
}