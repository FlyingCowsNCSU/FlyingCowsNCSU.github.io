// Define variables
const navToggle = document.querySelector(".nav-toggle");
const navLinks = document.querySelectorAll(".nav-link");

// Toggle navigation on click
navToggle.addEventListener("click", () => {
  document.body.classList.toggle("nav-open");
});

// Close navigation when a link is clicked
navLinks.forEach(link => {
  link.addEventListener("click", () => {
    document.body.classList.remove("nav-open");
  });
});
