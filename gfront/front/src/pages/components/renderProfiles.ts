import type { Profile } from "../scripts/profile";


export function renderProfiles(profiles: Profile[]): string {
  return profiles
    .map(
      (p) => `
    <div class="border p-4 rounded shadow flex gap-4 items-center bg-white">
      <img src="${p.avatar}" alt="${p.name}" class="w-12 h-12 rounded-full">
      <div>
        <h2 class="font-bold">${p.name}</h2>
        <p class="text-gray-600">${p.bio}</p>
      </div>
    </div>
  `
    )
    .join("");
}