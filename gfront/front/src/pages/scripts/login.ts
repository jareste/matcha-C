import { initPage } from "../components/initPage";
import { createBut, createDiv, createForm, createInp, createLab, createMain, createSpan, showError } from "./cssTools";

initPage("login", () => {
	const loginContainer = document.getElementById("login-container");

	if (!loginContainer)
	return;
	loginContainer.appendChild(renderLoginForm());

	const form = document.getElementById("loginForm") as HTMLFormElement;
	if (form) {
		setupLoginForm(form);
	}
});

function renderLoginForm()
{
	const main = createMain("flex flex-grow items-center justify-center p-2 bg-emerald-100");
	const main_div = createDiv("w-full max-w-xs");
	const form = createForm("loginForm", "px-8 pt-6 pb-8 mb-4");
	
	///
	const user_div = createDiv("mb-4 mt-2");
	const user_label = createLab("Username", "user", "block text-blue font-bold mb-2");
	const user_input = createInp("username","text", "user", "user","box-blue text-gray-500 border rounded w-full py-2 px-3 leading-tight focus:outline-none");
	user_input.setAttribute("required", "");
	
	///
	const pass_div = createDiv("mb-1");
	const pass_label = createLab("Password", "pass", "block text-blue font-bold mb-2");
	const pass_input = createInp("******************", "password", "pass", "pass", "box-blue text-gray-500 shadow border rounded w-full py-2 px-3 mb-3 leading-tight focus:outline-none");
	pass_input.setAttribute("required", "");
	const pass_span = createSpan("Enter your password","span_pass","text-red-500 text-xs italic hidden");
	
	///
	const div_userfail_wrap = createDiv("mb-6 h-2 w-45 text-sm font-medium");
	const div_userfail = createDiv("text-red-500 text-sm hidden animate-fade-in");
	div_userfail.setAttribute("id", "userfail");
	div_userfail.textContent = "Username or password incorrect";
	
	///
	const butt_div = createDiv("flex items-center justify-between mt-6 mb-2");
	const login_button = createBut("Log in", "loginBtnConfirm", " btn-contrast hover-underline-animation");
	login_button.setAttribute("type", "submit");
	
	///
	form.appendChild(user_div);
	user_div.appendChild(user_label);
	user_div.appendChild(user_input);
	
	///	
	form.appendChild(pass_div);
	pass_div.appendChild(pass_label);
	pass_div.appendChild(pass_input);
	pass_div.appendChild(pass_span);

	///
	form.appendChild(div_userfail_wrap);
	div_userfail_wrap.appendChild(div_userfail);
	
	form.appendChild(butt_div);
	butt_div.appendChild(login_button);


	main_div.appendChild(form);
	main.appendChild(main_div);

	return main;
}



export function setupLoginForm(loginForm: HTMLFormElement) {

	loginForm.addEventListener("submit", async (e) => {
	e.preventDefault();

	showError("userfail", "");
	showError("username-error-check", "");
	showError("pass-error-check", "");

	const formData = new FormData(loginForm);
	const username = (formData.get("user") as string)?.trim();
	const password = (formData.get("pass") as string) ?? "";

	let hasError = false;

	//
	if (!username) {
		showError("username-error-check", "Username is required");
		hasError = true;
	}

	//
	if (!password) {
		showError("pass-error-check", "Password is required");
		hasError = true;
	}

	if (hasError) return;

	try {
		const res = await fetch("/api/login", { //proper url
			method: "POST",
			body: JSON.stringify({
				username,
				password ,
			}),
			headers: {
				"Content-Type": "application/json",
			},
			credentials: "include",
		});

		if (!res.ok) {
		showError("userfail", "Invalid username or password");
		throw new Error(`Login failed - status ${res.status}`);
		}

		const data = await res.json();
		console.log("Success:", data);
		
		// store info init socket etc??

		//nav to dashboard or profile?

	} catch (err) {
		console.error("Error during login:", err);
		showError("userfail", "Login failed. Please try again.");
	}
	});
}
