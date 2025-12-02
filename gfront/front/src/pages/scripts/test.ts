import { initPage } from "../components/initPage";
import { createBtn, createDiv, createImage } from "./cssTools";

initPage("test", () => {
  const testContainer = document.getElementById("test-container");
  if (testContainer) {
    let test_img = ["https://images.unsplash.com/photo-1595871465907-19020bb76ad1?q=80&w=987&auto=format&fit=crop&ixlib=rb-4.1.0&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D","https://images.unsplash.com/photo-1587584867790-cf443d163e87?q=80&w=987&auto=format&fit=crop&ixlib=rb-4.1.0&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D","https://images.unsplash.com/photo-1593839686924-4b344fac3f8f?q=80&w=987&auto=format&fit=crop&ixlib=rb-4.1.0&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D"];
    testContainer.appendChild(carousel_reusable("test", test_img, "testcar"));
  }
});

//Preparing carousel and understanding functionalities.
//https://dev.to/min11benja/how-to-make-a-horizontal-moving-carousel-with-vanilla-js-ts-elc


// Basic reusable carousel hehe
// missing css, dot navigation and responsive
function carousel_reusable(name: string, img: string[], css_name: string)
{
    const carousel = createDiv(css_name + "-wrap");
    const carousel_track = createDiv(css_name + "-track");
    let i = 0;
    img.forEach(src => 
    {
      const carousel_slide = createDiv(css_name + "-slide");
      const carousel_img = createImage(src, "img_" + i, css_name + "-img");
      carousel_slide.appendChild(carousel_img);
      carousel_track.appendChild(carousel_slide);
      i++;
    });

    
    const carousel_btn_prev = createBtn("",name + "-prev", css_name + "-prev");

    const carousel_btn_next = createBtn("",name + "-next", css_name + "-next");

    carousel.appendChild(carousel_track);
    carousel.appendChild(carousel_btn_prev);
    carousel.appendChild(carousel_btn_next);

    return carousel;
}