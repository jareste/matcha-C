import { initPage } from "../components/initPage";
import { createDiv } from "./cssTools";

initPage("test", () => {
  const testContainer = document.getElementById("test-container");
  if (testContainer) {
    testContainer.appendChild(renderTestContent());
  }
});


function renderTestContent()
{
    const testmain = createDiv("");
    const sliderContainer = createDiv("slider-container");
    
    const innerslider = createDiv("inner-slider");
    sliderContainer.appendChild(innerslider);
    const card1 = createDiv("cardtest");
    const card2 = createDiv("cardtest");
    const card3 = createDiv("cardtest");
    const card4 = createDiv("cardtest");
    const card5 = createDiv("cardtest");
    const card6 = createDiv("cardtest");
    const card7 = createDiv("cardtest");

    innerslider.appendChild(card1);
    innerslider.appendChild(card2);
    innerslider.appendChild(card3);
    innerslider.appendChild(card4);
    innerslider.appendChild(card5);
    innerslider.appendChild(card6);
    innerslider.appendChild(card7);


    testmain.appendChild(sliderContainer);

    return testmain;
}