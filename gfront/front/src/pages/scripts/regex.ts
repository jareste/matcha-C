
export function checkValidUser(user: string): boolean
{
	const trimmedUser = user.trim();
	const userPattern = /^[a-zA-Z0-9]{2,16}$/;

	return userPattern.test(trimmedUser);
}

// .test is a function to test regex :)
export function checkValidEmail(email: string): boolean
{
	const trimmedEmail = email.trim();
	const emailPattern = /^((?!\.)[\w\-_.]*[^.])(@\w+)(\.\w+(\.\w+)?[^.\W])$/;

	return (emailPattern.test(trimmedEmail));
}


export function checkValidPass(password: string): boolean
{
	const trimmedPass = password.trim();
	const passPattern = /^(?=.*?[0-9])(?=.*?[a-zA-Z])(?=.*?[^0-9A-Za-z]).{7,32}$/;

	return (passPattern.test(trimmedPass));
}