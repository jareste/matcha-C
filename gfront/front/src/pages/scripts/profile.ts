
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
import { createBut, createDiv, createH2, createH3, createImage, createListItem, createP, createSpan, createUl, truncateText } from "./cssTools";

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
	const like = createBut("♥", "like", "text-green-500 text-4xl");
	const reject = createBut("✖", "reject", "text-red-500 text-4xl");
	

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

