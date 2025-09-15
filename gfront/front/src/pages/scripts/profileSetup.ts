import { initPage } from "../components/initPage";
import { createDiv, createInp, createLab, createP } from "./cssTools";

initPage("profile-setup", () => {
	const profileSetupContainer = document.getElementById("profile-setup-container");

	if (!profileSetupContainer)
		return;
	profileSetupContainer.appendChild(renderProfileSetup());


});



// Investigating drag & drop features. //https://www.youtube.com/watch?v=gjiu9kB7fQc //https://stackabuse.com/drag-and-drop-in-vanilla-javascript/ 
function renderProfileSetup(): HTMLDivElement {
	let draggedIndex: number | null = null;
  const main = createDiv("");

  const upload_group = createDiv("");
  const drop_zone = createDiv("drop-zone");
  const drop_text = createP("Drag & Drop images here or click to upload", "");

  const drop_inp = createInp("", "file", "file-input", "file-input", "");
  drop_inp.accept = "image/*";
  drop_inp.multiple = true;
  drop_inp.style.display = "none"; // hide visually

  const preview = createDiv("preview");
  preview.id = "preview";

  // Track selected images
  let images: File[] = [];

  // --- File Handling ---
  function handleFiles(files: FileList) {
    const validFiles = Array.from(files).filter(file => file.type.startsWith("image/"));
    images.push(...validFiles);
    renderPreviews();
  }

  function renderPreviews() {
    preview.innerHTML = ""; // clear previous previews

    images.forEach((file, index) => {
      const reader = new FileReader();

      reader.onload = (e) => {
        const wrapper = document.createElement("div");
		wrapper.draggable = true;
		wrapper.dataset.index = index.toString();
        wrapper.style.position = "relative";
        wrapper.style.display = "inline-block";
        wrapper.style.margin = "5px";
		wrapper.classList.add("cursor-move");

		wrapper.addEventListener("dragstart", () => {
  draggedIndex = index;
  wrapper.classList.add("opacity-50"); // visual feedback
});

wrapper.addEventListener("dragend", () => {
  wrapper.classList.remove("opacity-50");
  draggedIndex = null;
});

wrapper.addEventListener("dragover", (e) => {
  e.preventDefault(); // allow drop
});

wrapper.addEventListener("drop", (e) => {
  e.preventDefault();
  if (draggedIndex === null) return;

  const targetIndex = index;

  // Swap the files in the array
  const temp = images[draggedIndex];
  images[draggedIndex] = images[targetIndex];
  images[targetIndex] = temp;

  renderPreviews(); // re-render previews in new order
});

        const img = document.createElement("img");
        img.src = e.target?.result as string;
        img.style.maxWidth = "120px";
        img.style.maxHeight = "120px";
        img.style.borderRadius = "4px";
        img.style.display = "block";

        const removeBtn = document.createElement("button");
        removeBtn.innerText = "✖";
        removeBtn.style.position = "absolute";
        removeBtn.style.top = "2px";
        removeBtn.style.right = "2px";
        removeBtn.style.background = "rgba(0,0,0,0.6)";
        removeBtn.style.color = "white";
        removeBtn.style.border = "none";
        removeBtn.style.borderRadius = "50%";
        removeBtn.style.cursor = "pointer";
        removeBtn.style.width = "24px";
        removeBtn.style.height = "24px";

        removeBtn.addEventListener("click", () => {
          images.splice(index, 1); // remove from array
          renderPreviews();        // re-render
        });

        wrapper.appendChild(img);
        wrapper.appendChild(removeBtn);
        preview.appendChild(wrapper);
      };

      reader.readAsDataURL(file);
    });
  }

  // --- Click to open file picker ---
  drop_zone.addEventListener("click", () => drop_inp.click());
  drop_inp.addEventListener("change", () => {
    if (drop_inp.files) handleFiles(drop_inp.files);
    drop_inp.value = ""; // allow re-selecting same file later
  });

  // --- Drag & Drop ---
  drop_zone.addEventListener("dragover", (e) => {
    e.preventDefault();
    e.stopPropagation();
    drop_zone.classList.add("dragover");
  });

  drop_zone.addEventListener("dragleave", (e) => {
    e.preventDefault();
    e.stopPropagation();
    drop_zone.classList.remove("dragover");
  });

  drop_zone.addEventListener("drop", (e) => {
    e.preventDefault();
    e.stopPropagation();
    drop_zone.classList.remove("dragover");

    if (e.dataTransfer?.files) handleFiles(e.dataTransfer.files);
  });

  // Build DOM
  main.appendChild(upload_group);
  upload_group.appendChild(drop_zone);
  drop_zone.appendChild(drop_text);
  drop_zone.appendChild(drop_inp);
  main.appendChild(preview);

  return main;
}

// https://flowbite.com/docs/forms/file-input/

// function renderProfileSetup()
// {
// 	// `
// 	// <div id="drop-zone" class="drop-zone">
// 	//   <p>Drag & Drop images here or click to upload</p>
// 	//   <input id="file-input" type="file" accept="image/*" multiple hidden />
// 	// </div>
// 	// <div id="preview" class="preview"></div>
// 	// `
// 	const main = createDiv("");

// 	const upload_group = createDiv("");
// 	const drop_zone = createLab("Drag & Drop images here or click to upload","drop-zone","");
	
// 	const drop_inp = createInp("","file", "file-input", "file-input", "")
// 	drop_inp.accept = "image/*";
// 	drop_inp.multiple = true;
// 	drop_inp.hidden = false;
// 	drop_inp.style.display = "none";

// 	const preview = createDiv("preview");
// 	preview.id = "preview";

// 	main.appendChild(upload_group);
	
// 	upload_group.appendChild(drop_zone);
// 	drop_zone.appendChild(drop_inp);

	
// 	main.appendChild(preview);
	
// 	drop_zone.addEventListener("click", () => drop_inp.click());
// 	drop_zone.addEventListener("dragover", (e) => {
//   e.preventDefault();
//   e.stopPropagation();
//   drop_zone.classList.add("dragover");
// });

// drop_zone.addEventListener("dragleave", (e) => {
//   e.preventDefault();
//   e.stopPropagation();
//   drop_zone.classList.remove("dragover");
// });

// drop_zone.addEventListener("drop", (e) => {
//   e.preventDefault();
//   e.stopPropagation();
//   drop_zone.classList.remove("dragover");

//   if (e.dataTransfer?.files) {
//     handleFiles(e.dataTransfer.files, preview);
//   }
// });

// 	return main;
// }