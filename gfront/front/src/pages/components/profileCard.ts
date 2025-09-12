// <div class="h-screen dark:bg-gray-700 bg-gray-200 pt-12">

//     <!-- Card start -->
//         <div class="max-w-sm mx-auto bg-white dark:bg-gray-900 rounded-lg overflow-hidden shadow-lg">
//             <div class="border-b px-4 pb-6">
//                 <div class="text-center my-4">
//                     <img class="h-32 w-32 rounded-full border-4 border-white dark:border-gray-800 mx-auto my-4"
//                         src="https://randomuser.me/api/portraits/women/21.jpg" alt="">
//                     <div class="py-2">
//                         <h3 class="font-bold text-2xl text-gray-800 dark:text-white mb-1">Cait Genevieve</h3>
//                         <div class="inline-flex text-gray-700 dark:text-gray-300 items-center">
//                             <svg class="h-5 w-5 text-gray-400 dark:text-gray-600 mr-1" fill="currentColor"
//                                 xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
//                                 <path class=""
//                                     d="M5.64 16.36a9 9 0 1 1 12.72 0l-5.65 5.66a1 1 0 0 1-1.42 0l-5.65-5.66zm11.31-1.41a7 7 0 1 0-9.9 0L12 19.9l4.95-4.95zM12 14a4 4 0 1 1 0-8 4 4 0 0 1 0 8zm0-2a2 2 0 1 0 0-4 2 2 0 0 0 0 4z" />
//                             </svg>
//                             New York, NY
//                         </div>
//                     </div>
//                 </div>
//                 <div class="flex gap-2 px-2">
//                     <button
//                         class="flex-1 rounded-full bg-blue-600 dark:bg-blue-800 text-white dark:text-white antialiased font-bold hover:bg-blue-800 dark:hover:bg-blue-900 px-4 py-2">
//                         Follow
//                     </button>
//                     <button
//                         class="flex-1 rounded-full border-2 border-gray-400 dark:border-gray-700 font-semibold text-black dark:text-white px-4 py-2">
//                         Message
//                     </button>
//                 </div>
//             </div>
//             <div class="px-4 py-4">
//                 <div class="flex gap-2 items-center text-gray-800 dark:text-gray-300 mb-4">
//                     <svg class="h-6 w-6 text-gray-600 dark:text-gray-400" fill="currentColor"
//                         xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
//                         <path class=""
//                             d="M12 12a5 5 0 1 1 0-10 5 5 0 0 1 0 10zm0-2a3 3 0 1 0 0-6 3 3 0 0 0 0 6zm9 11a1 1 0 0 1-2 0v-2a3 3 0 0 0-3-3H8a3 3 0 0 0-3 3v2a1 1 0 0 1-2 0v-2a5 5 0 0 1 5-5h8a5 5 0 0 1 5 5v2z" />
//                     </svg>
//                     <span><strong class="text-black dark:text-white">12</strong> Followers you know</span>
//                 </div>
//                 <div class="flex">
//                     <div class="flex justify-end mr-2">
//                         <img class="border-2 border-white dark:border-gray-800 rounded-full h-10 w-10 -mr-2"
//                             src="https://randomuser.me/api/portraits/men/32.jpg" alt="">
//                         <img class="border-2 border-white dark:border-gray-800 rounded-full h-10 w-10 -mr-2"
//                             src="https://randomuser.me/api/portraits/women/31.jpg" alt="">
//                         <img class="border-2 border-white dark:border-gray-800 rounded-full h-10 w-10 -mr-2"
//                             src="https://randomuser.me/api/portraits/men/33.jpg" alt="">
//                         <img class="border-2 border-white dark:border-gray-800 rounded-full h-10 w-10 -mr-2"
//                             src="https://randomuser.me/api/portraits/women/32.jpg" alt="">
//                         <img class="border-2 border-white dark:border-gray-800 rounded-full h-10 w-10 -mr-2"
//                             src="https://randomuser.me/api/portraits/men/44.jpg" alt="">
//                         <img class="border-2 border-white dark:border-gray-800 rounded-full h-10 w-10 -mr-2"
//                             src="https://randomuser.me/api/portraits/women/42.jpg" alt="">
//                         <span
//                             class="flex items-center justify-center bg-white dark:bg-gray-800 text-sm text-gray-800 dark:text-white font-semibold border-2 border-gray-200 dark:border-gray-700 rounded-full h-10 w-10">
//                             +999
//                         </span>
//                     </div>
//                 </div>
//             </div>
//         </div>
//     <!-- Card end -->

// </div>


// deleted old idea.
// preparing new design with new base

/* Trascendence profle port. might reuse with new ideas.
export function createProfileCard(name:string, user:User)
{
	const body = document.getElementById(name);
	if (!body) return;

	body.innerHTML = ``;

	const main = createDiv("flex flex-grow items-center max-w-3xl justify-center");
	const card_div = createDiv("bg-gradient-border w-full mx-auto p-6 rounded-lg text-white shadow-lg");


	//
	const card_div_img = createDiv(" mt-4 mx-auto w-32 h-32 relative border-4 border-neonBlue rounded-full overflow-hidden bg-bgDark");
	const card_img_profpic = createImage(user?.avatar || "/default.jpg", "profile picture", "object-cover w-full h-full");

	//
	const card_details_user = createDiv("text-center mt-4 px-6");
	const card_user = createSpan(user?.username || "Username", "user", "text-neonBlue text-xl font-bold block truncate");
	card_user.title = user?.username;
	const card_details_email = createDiv("text-center mt-2 mb-6");
	const tmp_email = truncateText(user?.email,13);
	const card_email = createSpan(tmp_email || "example@email.com", "email", "text-white truncate");
	card_email.title = user?.email;

	//
	const card_stats = createUl("py-4 border-t border-b border-neonPink border-opacity-30 flex items-center justify-around");
	
	const card_wins_li = createListItem("", "flex flex-col items-center px-2");
	const card_wins = createSpan("Wins", "", "text-neonPink2 font-bold mb-1");
	const card_wins_val = createSpan(user?.wins || "0", "wins", "text-white");

	const card_losses_li = createListItem("", "flex flex-col items-center px-2");
	const card_losses = createSpan("Losses", "", "text-neonPink2 font-bold mb-1");
	const card_losses_val = createSpan(user?.losses || "0", "losses", "text-white");


	//
	const card_butt_div = createDiv("p-4");
	const card_butt = createBut("More Stats", "extra-profile", "btn-neon-pink2 w-full hover-underline-animation");

	//
	body.appendChild(main);
	main.appendChild(card_div);

	//
	card_div.appendChild(card_div_img);
	card_div_img.appendChild(card_img_profpic);

	card_div.appendChild(card_details_user);
	card_details_user.appendChild(card_user);

	card_div.appendChild(card_details_email);
	card_details_email.appendChild(card_email);

	card_div.appendChild(card_stats);

	card_stats.appendChild(card_wins_li);
	card_wins_li.appendChild(card_wins);
	card_wins_li.appendChild(card_wins_val);
	
	card_stats.appendChild(card_losses_li);
	card_losses_li.appendChild(card_losses);
	card_losses_li.appendChild(card_losses_val);
	
	//
	card_div.appendChild(card_butt_div);
	card_butt_div.appendChild(card_butt);
	
	//
	const extraBtnCard = document.getElementById("extra-profile");
	extraBtnCard?.addEventListener("click", () => {
		createExtraProfile("expandCard", user);
	});
}

///
export function createExtraProfile(name:string, user:User)
{
	const body = document.getElementById(name);
	if (!body) return;

	body.innerHTML = "";

	//
	const wins = parseInt(user?.wins || "0");
	const losses = parseInt(user?.losses || "0");
	const gamesPlayed = wins + losses;
	const winRate = gamesPlayed > 0 ? parseFloat(((wins / gamesPlayed) * 100).toFixed(2)) : 0;


	const main = createDiv("flex flex-grow items-center max-w-3xl justify-center");
	const card_div = createDiv("bg-gradient-border w-full mx-auto p-6 rounded-lg text-white shadow-lg");
	
	//
	const card_details_user = createDiv("text-center mb-6");
	const card_user = createSpan(user?.username || "Username", "user", "text-neonBlue text-xl font-bold truncate");
	card_user.title = user?.username;
	
	//
	const card_stats = createUl("py-4 border-b border-neonPink flex items-center justify-around");
	
	const card_winRate_li = createListItem("", "flex flex-col items-center");
	const card_winRate = createSpan("Win Rate", "", "text-neonPink2 font-bold mb-1");
	const card_winRate_val = createSpan(`${winRate}%`, "winRate", "text-white");
	
	const card_gamesPlayed_li = createListItem("", "flex flex-col items-center");
	const card_gamesPlayed = createSpan("Games Played", "", "text-neonPink2 font-bold mb-1");
	const card_gamesPlayed_val = createSpan(gamesPlayed.toString(), "gamesPlayed", "text-white");
	
	const card_longestRally_li = createListItem("", "flex flex-col items-center");
	const card_longestRally = createSpan("Longest Rally", "", "text-neonPink2 font-bold mb-1");
	const card_longestRally_val = createSpan(user?.longestRally || "0", "longestRally", "text-white");
	
	//
	const card_stats2 = createUl("py-4 border-b border-neonPink flex items-center justify-around");
	
	const card_parryAttempts_li = createListItem("", "flex flex-col items-center");
	const card_parryAttempts = createSpan("Parry Attempts", "", "text-neonPink2 font-bold mb-1");
	const card_parryAttempts_val = createSpan(user?.parryAttempts || "0", "parryAttempts", "text-white");
	
	const card_parryAccuracy_li = createListItem("", "flex flex-col items-center");
	const card_parryAccuracy = createSpan("Parry Accuracy", "", "text-neonPink2 font-bold mb-1");
	
	let tmp_1 = (100* parseFloat(user.parryCount) / parseFloat(user.parryAttempts)).toFixed(2);
	if (parseFloat(user.parryAttempts) == 0)
		tmp_1 = "0";
	const card_parryAccuracy_val = createSpan(user?.parryCount ? `${tmp_1}%` : "0%", "parryAccuracy", "text-white");
	
	const card_longestParryChain_li = createListItem("", "flex flex-col items-center");
	const card_longestParryChain = createSpan("Longest Parry Chain", "", "text-neonPink2 font-bold mb-1");
	const card_longestParryChain_val = createSpan(user?.longestParryChain || "0", "longestParryChain", "text-white");

	//
	const card_stats3 = createUl("py-4 flex items-center justify-around");
	
	const card_timesScored_li = createListItem("", "flex flex-col items-center");
	const card_timesScored = createSpan("Times Scored", "", "text-neonPink2 font-bold mb-1");
	const card_timesScored_val = createSpan(user?.timesScored || "0", "timesScored", "text-white");
	
	const card_timesScoredAgainst_li = createListItem("", "flex flex-col items-center");
	const card_timesScoredAgainst = createSpan("Times Scored Against", "", "text-neonPink2 font-bold mb-1");
	const card_timesScoredAgainst_val = createSpan(user?.timesScoredAgainst || "0", "timesScoredAgainst", "text-white");
	
	const card_wins_li = createListItem("", "flex flex-col items-center");
	const card_wins = createSpan("Wins", "", "text-neonPink2 font-bold mb-1");
	const card_wins_val = createSpan(wins.toString(), "wins", "text-white");
	
	//
	const card_butt_div = createDiv("pt-4 mb-4 mt-4 border-t border-neonPink");
	const card_butt = createBut("Back", "main-profile", "btn-neon-pink2 w-full hover-underline-animation");

	//
	body.appendChild(main);
	main.appendChild(card_div);
	card_div.appendChild(card_details_user);
	card_details_user.appendChild(card_user);

	//
	card_div.appendChild(card_stats);
	card_div.appendChild(card_stats2);
	card_div.appendChild(card_stats3);

	//
	card_stats.appendChild(card_winRate_li);
	card_winRate_li.appendChild(card_winRate);
	card_winRate_li.appendChild(card_winRate_val);
	
	card_stats.appendChild(card_gamesPlayed_li);
	card_gamesPlayed_li.appendChild(card_gamesPlayed);
	card_gamesPlayed_li.appendChild(card_gamesPlayed_val);
	
	card_stats.appendChild(card_longestRally_li);
	card_longestRally_li.appendChild(card_longestRally);
	card_longestRally_li.appendChild(card_longestRally_val);

	//
	card_stats2.appendChild(card_parryAttempts_li);
	card_parryAttempts_li.appendChild(card_parryAttempts);
	card_parryAttempts_li.appendChild(card_parryAttempts_val);
	
	card_stats2.appendChild(card_parryAccuracy_li);
	card_parryAccuracy_li.appendChild(card_parryAccuracy);
	card_parryAccuracy_li.appendChild(card_parryAccuracy_val);
	
	card_stats2.appendChild(card_longestParryChain_li);
	card_longestParryChain_li.appendChild(card_longestParryChain);
	card_longestParryChain_li.appendChild(card_longestParryChain_val);

	//
	card_stats3.appendChild(card_timesScored_li);
	card_timesScored_li.appendChild(card_timesScored);
	card_timesScored_li.appendChild(card_timesScored_val);
	
	card_stats3.appendChild(card_timesScoredAgainst_li);
	card_timesScoredAgainst_li.appendChild(card_timesScoredAgainst);
	card_timesScoredAgainst_li.appendChild(card_timesScoredAgainst_val);
	
	card_stats3.appendChild(card_wins_li);
	card_wins_li.appendChild(card_wins);
	card_wins_li.appendChild(card_wins_val);

	//
	card_div.appendChild(card_butt_div);
	card_butt_div.appendChild(card_butt);

	//
	const mainBtnCard = document.getElementById("main-profile");
	mainBtnCard?.addEventListener("click", () => {
		createProfileCard("expandCard", user);
	});
}


*/