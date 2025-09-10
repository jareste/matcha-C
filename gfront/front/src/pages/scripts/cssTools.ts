// Remember we can always add new attributes with thing.setAttribute() 


export function createDiv(className:string)
{
	const div: HTMLDivElement = document.createElement('div');
	div.className = className;
	return div;
}

export function createLab(text: string, id: string | null, css: string)
{
	const label = document.createElement('label');
	label.textContent = text;
	label.className = css;
	if (id) label.htmlFor = id;
	return label;
}

export function createInp(placeholder:string, type:string, name:string, id:string, css:string)
{
	const input: HTMLInputElement = document.createElement('input');
	input.className = css;
	input.type = type;
	input.placeholder = placeholder;
	input.id = id;
	input.name = name;	
	return input;
}

export function createSpan(text:string, id:string, css:string)
{
	const span: HTMLSpanElement = document.createElement('span');
	span.className = css;
	span.id = id;
	span.textContent = text;
	return span;
}

export function createForm(id:string, css:string)
{
	const form:HTMLFormElement = document.createElement('form');
	form.id = id;
	form.className = css;
	return form;
}

export function createUl(css:string)
{
	const unorderedlist:HTMLUListElement = document.createElement('ul');
	unorderedlist.className = css;
	return unorderedlist;
}

export function createListItem(text:string, css:string)
{
	const listItem:HTMLLIElement = document.createElement('li');
	listItem.textContent = text;
	listItem.className = css;
	return listItem;
}

export function createBut(text:string, id:string, css:string)
{
	const button:HTMLButtonElement = document.createElement('button');
	button.textContent = text;
	button.id = id;
	button.className = css;
	return button;
}

export function createMain(css:string)
{
	const main:HTMLElement = document.createElement('main');
	main.className = css;
	return main;
}

export function createImage(path:string, alt:string, css:string)
{
	const image:HTMLImageElement = document.createElement('img');
	image.src = path;
	image.alt = alt;
	image.className = css;
	return image;
}

export function createP(text:string, css:string)
{
	const paragraph:HTMLParagraphElement = document.createElement('p');
	paragraph.textContent = text;
	paragraph.className = css;
	return paragraph;
}

export function createH2(text:string, css:string)
{
	const h2:HTMLHeadingElement = document.createElement('h2');
	h2.textContent = text;
	h2.className = css;
	return h2;
}

export function createH3(text:string, css:string)
{
	const h3:HTMLHeadingElement = document.createElement('h3');
	h3.textContent = text;
	h3.className = css;
	return h3;
}


/*
<form class="max-w-sm mx-auto">
  <label for="countries" class="block mb-2 text-sm font-medium text-gray-900 dark:text-white">Select an option</label>
  <select id="countries" class="bg-gray-50 border border-gray-300 text-gray-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full p-2.5 dark:bg-gray-700 dark:border-gray-600 dark:placeholder-gray-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500">
	<option selected>Choose a country</option>
	<option value="US">United States</option>
	<option value="CA">Canada</option>
	<option value="FR">France</option>
	<option value="DE">Germany</option>
  </select>
</form>
*/


export function createSelect(id: string, css: string, options: {value: string, text: string}[])
{
	const select:HTMLSelectElement = document.createElement('select');
	select.id = id;
	select.name = id;
	select.className = css;

	options.forEach(option => {
		const opt = document.createElement('option');
		opt.value = option.value;
		opt.textContent = option.text;
		select.appendChild(opt);
	});

	return select;
}


// Too hard to implment - To be explored after evaluation ;)
// function greenRing(input)
// {
// 	if (input.value)
// 	{
// 		input.classList.add('ring-2', 'ring-green-500');
// 	}
// 	else
// 	{
// 		input.classList.remove('ring-2', 'ring-green-500');
// 	}
// }

 
export function showError(elementId: string, message: string)
{
	const element = document.getElementById(elementId);
	if (!element) return;
	
	element.textContent = '';
	void element.offsetWidth;
	element.textContent = message;
	element.classList.remove("hidden");
}

export function truncateText(text: string, maxLength: number): string
{
	if (text.length <= maxLength)
	{
		return text;
	}
	return text.slice(0, maxLength) + "…";
}