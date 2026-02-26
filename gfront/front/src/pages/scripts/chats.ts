import { initPage } from "../components/initPage";
import { createBtn, createDiv, createImage, createInp } from "./tools";


initPage("chats", () => {
  const chatsContainer = document.getElementById("chats-container");
  if (chatsContainer) {
    chatsContainer.innerHTML = renderAllChats();
  }
});


// Socket depenant refresh?
// where do i pull the info?

//Try create a reusable chat.

// https://github.com/GuillemFo/ft_transcendence/blob/main/front/app_vite/src/matchHistory.ts

function renderPrivateChat(container_id:string, user_id:string, chat_id:string)
{
	//protect if element exist
	const chatContainer = document.getElementById(container_id);	// All chat will expand here

	const privateChatWrap = createDiv("");

	const privateChatInnerWrap= createDiv("");	// dont know if ill need it

	const privateChatHead = createDiv("");

	const privateChatHeadImg = createImage("", "", "", "");
	
	const privateChatHeadName = createDiv("");

	const privateChatBody = createDiv("");

	// a function to pull all chat history sorted by time and move user to latest messages
	let i = 0;
	// for userChatHistory_id items
	{
		const msg = createDiv("");
		msg.id = `${userChatHistory_id + i}`;
	
		const msg_date = createDiv("");

		const msg_content = createDiv("");
		i++;
		// how do i print incoming or sent messages? need a new function? Kowalski, ideas!!!
	}
	//force scroll down
	const chatInputWrap = createDiv("");
	const chatInputBox = createInp("", "", "", "", "");
	const chatInputSendBtn = createBtn("", "", "");
	//proper function to send msg, backend, etc...
}



function renderAllChats()
{
	const mainWrap = createDiv("");		// outer layer

	const innerWrap = createDiv("");	// inner layer

	const chat_nav = createDiv("");		// buttons for nav in and out the chats. This will contain home button to render all chats again and options to edit, delete block for all elements/chats // this will need its time and will come after chats work properly ;)

	const allChats = createDiv("");		// general wrap for all contacts chats (whatsapp style) sorted by last message received or sent
	allChats.id = "MainChats";			// This is so we can do getelementbyid and re render the private chat inside

	// for user.match or confirmed_match or whatever list of contacts
	{
		// wrapper button for the contact so on click, chat opens and replaces allChats for that contact chat (whatsapp style)
		const contactWrap = createDiv("");
		contactWrap.id = `${contact.id}`;	// do i need this?
		const contactImg = createImage("","","","");
		const contactName = createDiv("");
		//contactWrap.addEventListener("click", () => {
		//	renderPrivateChat("MainChats", user_id or token, chat_id / contact_id or both);
		//});
	}

	// Main chats list
	// Private & clickable chats
	// Open profiles
	// Manage chats?

	// socket.io?

	return '';
}