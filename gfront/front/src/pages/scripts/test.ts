import { initPage } from "../components/initPage";
import { createBtn, createDiv, createImage } from "./tools";

initPage("test", () => {
  const testContainer = document.getElementById("test-container");
  if (testContainer) {
	let test_img = ["https://images.unsplash.com/photo-1595871465907-19020bb76ad1?q=80&w=987&auto=format&fit=crop&ixlib=rb-4.1.0&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D","https://images.unsplash.com/photo-1587584867790-cf443d163e87?q=80&w=987&auto=format&fit=crop&ixlib=rb-4.1.0&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D","https://images.unsplash.com/photo-1593839686924-4b344fac3f8f?q=80&w=987&auto=format&fit=crop&ixlib=rb-4.1.0&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D"];
	testContainer.appendChild(carousel_reusable("test", test_img, "matchcar"));
  }
});

//Preparing carousel and understanding functionalities.
//https://dev.to/min11benja/how-to-make-a-horizontal-moving-carousel-with-vanilla-js-ts-elc

//https://www.w3schools.com/csS/css3_object-fit.asp



// Basic reusable carousel
// missing dot navigation and responsive
// write the css in each place without depending of .css file ??
function carousel_reusable(name: string, img: string[], css_name: string)
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

