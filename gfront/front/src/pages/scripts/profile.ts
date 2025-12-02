
export interface Profile
{
	id: string; //UUID?
	username: string;
	email: string;
	first_name: string;
	last_name: string;
	gender?: string;
	age?: string;
	orientation?: string;
	bio?: string;
	fame_rating?: string;
	gps_lat?: string;
	gps_lon?: string;
	location_optout?: boolean;
	last_online?: string;
	profpic?: string;
	images?: string[];
	hashtags?: string[];
}

const testprofile: Profile = 
{
	"id": "550e8400-e29b-41d4-a716-446655440000",
	"username": "jane_doe42",
	"email": "jane.doe@example.com",
	"first_name": "Jane",
	"last_name": "Doe",
	"gender": "Female",
	"age": "28",
	"orientation": "Bisexual",
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




import { initPage } from "../components/initPage";
import { createBtn, createDiv, createH2, createH3, createImage, createListItem, createP, createSpan, createSvgBut, createUl, truncateText } from "./cssTools";

initPage("profile", () => {
	const profileContainer = document.getElementById("profile-container");
	if (profileContainer) {
	profileContainer.appendChild(renderProfileContent(testprofile));
	}
});


function renderProfileContent(profile: Profile)
{

	const card = createDiv("container mx-auto rounded-2xl overflow-hidden shadow-lg bg-white border");

	const imgWrapper = createDiv("overflow-hidden");
	const img = createImage(`${profile.profpic}`,`${profile.first_name} ${profile.last_name}`,"w-full h-full object-cover object-center");
	imgWrapper.appendChild(img);


	const info = createDiv("p-4");


	const header = createDiv("flex items-center gap-2");
	const name = createH2(`${profile.first_name} ${profile.last_name}`,"text-xl font-semibold");
	const age = createSpan(`${profile.age}`, "", "text-gray-500 text-lg");
	header.appendChild(name);
	header.appendChild(age);


	// const meta = createDiv("text-sm text-gray-600 mt-1"); Pending
	// meta.textContent = `• ⭐ ${profile.fame_rating}`;


	const orientation = createP(`${profile.gender} • ${profile.orientation}`,"text-gray-600 text-sm mt-1");


	const aboutWrapper = createDiv("mt-4");
	const aboutTitle = createH3("About me", "font-semibold text-gray-800");
	const aboutText = createP(`${profile.bio}`,"text-gray-600 text-sm mt-1 leading-relaxed");
	aboutWrapper.appendChild(aboutTitle);
	aboutWrapper.appendChild(aboutText);


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

	const buttonsWrapper = createDiv("mt-4 mb-4 flex justify-evenly");
	
	const likeSVG = 
	`
		<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
			<path d="M3.48877 6.00387C2.76311 7.24787 2.52428 8.97403 2.97014 10.7575C3.13059 11.3992 3.59703 12.2243 4.33627 13.174C5.06116 14.1052 5.9864 15.0787 6.96636 16.0127C8.90945 17.8648 11.0006 19.4985 12 20.254C12.9994 19.4985 15.0905 17.8648 17.0336 16.0127C18.0136 15.0787 18.9388 14.1052 19.6637 13.174C20.403 12.2243 20.8694 11.3992 21.0299 10.7575C21.4757 8.97403 21.2369 7.24788 20.5112 6.00387C19.8029 4.78965 18.6202 4 17 4C15.5904 4 14.5969 5.04228 13.8944 6.44721C13.5569 7.12228 13.3275 7.80745 13.1823 8.33015C13.1102 8.58959 13.0602 8.80435 13.0286 8.95172C12.9167 9.47392 12.3143 9.5 12 9.5C11.6857 9.5 11.0823 9.46905 10.9714 8.95172C10.9398 8.80436 10.8898 8.58959 10.8177 8.33015C10.6725 7.80745 10.4431 7.12229 10.1056 6.44722C9.40308 5.04228 8.40956 4 6.99998 4C5.37979 4 4.19706 4.78965 3.48877 6.00387ZM12 5.77011C12.0341 5.69784 12.0693 5.62535 12.1056 5.55279C12.9031 3.95772 14.4096 2 17 2C19.3798 2 21.1971 3.21035 22.2388 4.99613C23.2631 6.75212 23.5243 9.02597 22.9701 11.2425C22.7076 12.2927 22.0354 13.3832 21.2419 14.4025C20.4341 15.4402 19.4327 16.4891 18.4135 17.4605C16.3742 19.4042 14.1957 21.1022 13.181 21.8683C12.4803 22.3974 11.5197 22.3974 10.819 21.8683C9.80433 21.1022 7.62583 19.4042 5.58648 17.4605C4.56733 16.4891 3.56586 15.4402 2.75806 14.4025C1.96461 13.3832 1.2924 12.2927 1.02986 11.2425C0.475714 9.02597 0.736884 6.75213 1.76121 4.99613C2.80291 3.21035 4.62017 2 6.99998 2C9.59038 2 11.0969 3.95772 11.8944 5.55278C11.9307 5.62535 11.9659 5.69784 12 5.77011Z"></path>
		</svg>
	`;

	const like = createSvgBut("like", "text-green-500 text-4xl", likeSVG);
	
	const rejectSVG = 
	`
		<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
  			<path d="M20.7457 3.32851C20.3552 2.93798 19.722 2.93798 19.3315 3.32851L12.0371 10.6229L4.74275 3.32851C4.35223 2.93798 3.71906 2.93798 3.32854 3.32851C2.93801 3.71903 2.93801 4.3522 3.32854 4.74272L10.6229 12.0371L3.32856 19.3314C2.93803 19.722 2.93803 20.3551 3.32856 20.7457C3.71908 21.1362 4.35225 21.1362 4.74277 20.7457L12.0371 13.4513L19.3315 20.7457C19.722 21.1362 20.3552 21.1362 20.7457 20.7457C21.1362 20.3551 21.1362 19.722 20.7457 19.3315L13.4513 12.0371L20.7457 4.74272C21.1362 4.3522 21.1362 3.71903 20.7457 3.32851Z"/>
		</svg>
	`;
	const reject = createSvgBut("reject", "text-red-500 text-4xl", rejectSVG);

	info.appendChild(header);
	// info.appendChild(meta);
	info.appendChild(orientation);
	info.appendChild(aboutWrapper);
	info.appendChild(tagsWrapper);

	buttonsWrapper.appendChild(reject);
	buttonsWrapper.appendChild(like);

	card.appendChild(imgWrapper);
	card.appendChild(info);
	card.appendChild(buttonsWrapper);

	return card;
}

