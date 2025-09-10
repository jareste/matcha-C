
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

// export const profiles: Profile[] = [
//   { id: 1, name: "Alice", avatar: "/assets/alice.png", bio: "Loves coding" },
//   { id: 2, name: "Bob", avatar: "/assets/bob.png", bio: "Enjoys hiking" },
//   { id: 3, name: "Charlie", avatar: "/assets/charlie.png", bio: "Gamer" },
// ];

import { initPage } from "../components/initPage";

initPage("profile", () => {
  const profileContainer = document.getElementById("profile-container");
  if (profileContainer) {
	profileContainer.innerHTML = renderProfileContent();
  }
});


function renderProfileContent()
{
	// work with proper ts builders from cssTools
	return '';
}