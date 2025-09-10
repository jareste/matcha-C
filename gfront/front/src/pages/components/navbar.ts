export function renderNavbar(active: string): string
{
	return `
		<nav class="flex gap-4 p-4 bg-white shadow">
			<a href="index" class="${active === "home" ? "text-blue-600" : ""}">Home</a>
			<a href="browse" class="${active === "browse" ? "text-blue-600" : ""}">Browse</a>
			<a href="profile" class="${active === "profile" ? "text-blue-600" : ""}">Profile</a>
			<a href="chats" class="${active === "chats" ? "text-blue-600" : ""}">Chat</a>
			<a href="login" class="${active === "login" ? "text-blue-600" : ""}">Login</a>
			<a href="register" class="${active === "register" ? "text-blue-600" : ""}">Register</a>
			<a href="edit-profile" class="${active === "edit" ? "text-blue-600" : ""}">Edit</a>
		</nav>
	`;
}
