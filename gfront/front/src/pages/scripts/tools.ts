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

export function createTextArea(id:string, name:string, css:string)
{
	const input: HTMLTextAreaElement = document.createElement('textarea');
	input.className = css;
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

export function createBtn(text:string, id:string, css:string)
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

export function createImage(path:string, id:string, alt:string, css:string)
{
	const image:HTMLImageElement = document.createElement('img');
	image.src = path;
	image.id = id;
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

export function createSvgBut(id: string, css: string, svg: string)
{
	const button:HTMLButtonElement = document.createElement('button');
	button.id = id;
	button.className = css;
	button.innerHTML = svg;
	
	// this should help apply size like text to svg
	const svgEl = button.querySelector('svg');
	if (svgEl)
	{
		svgEl.setAttribute('fill', 'currentColor');
		svgEl.setAttribute('width', '1em');
		svgEl.setAttribute('height', '1em');
	}


	return button;
}


export function carousel_reusable(name: string, img: string[], css_name: string)
{
	const carousel = createDiv(css_name + "-wrap");
	const carousel_track = createDiv(css_name + "-track");
	
	let currentIndex = 0;
	let i = 0;
	img.forEach(src => 
	{
	  const carousel_slide = createDiv(css_name + "-slide");
	  const carousel_img = createImage(src, "img_" + i, "profile_" + i, css_name + "-img"); // check if multiple img with same name will colide...
	  carousel_slide.appendChild(carousel_img);
	  carousel_track.appendChild(carousel_slide);
	  i++;
	});

	
	const carousel_btn_prev = createBtn("",name + "-prev", css_name + "-prev");

	const carousel_btn_next = createBtn("",name + "-next", css_name + "-next");

	carousel.appendChild(carousel_track);
	carousel.appendChild(carousel_btn_prev);
	carousel.appendChild(carousel_btn_next);
	
	//https://www.w3schools.com/jsref/prop_element_clientwidth.asp
	//https://www.w3schools.com/jsref/tryit.asp?filename=tryjsref_element_clientheight
	
	function updateCarousel()
	{
		const slideWidth = carousel.clientWidth;
		carousel_track.style.transform = `translateX(-${currentIndex * slideWidth}px)`;
	}
	
	
	function nextSlide()
	{
		if (currentIndex < img.length - 1)
		{
			currentIndex++;
		}
		else
		{
			currentIndex = 0;
		}
		updateCarousel();
	}

	function prevSlide()
	{
		if (currentIndex > 0)
		{
			currentIndex--;
		}
		else
		{
			currentIndex = img.length - 1;
		}
		updateCarousel();
	}
	
	carousel_btn_next.onclick = nextSlide;
	carousel_btn_prev.onclick = prevSlide;

	return carousel;
}