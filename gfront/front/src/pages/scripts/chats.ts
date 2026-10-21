import { initPage } from "../components/initPage";


initPage("chats", () => {
  const chatsContainer = document.getElementById("chats-container");
  if (chatsContainer) {
    chatsContainer.innerHTML = renderChatsContent();
  }
});


function renderChatsContent()
{

	// Main chats list
	// Private & clickable chats
	// Open profiles
	// Manage chats?

	// socket.io?

	return '';
}