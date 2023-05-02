// async function openTab(evt, tabName) {
//   var i, tabContent, tabs;

//   // Hide all tab content
//   tabContent = document.getElementsByClassName("tab-content");
//   for (i = 0; i < tabContent.length; i++) {
//     tabContent[i].style.display = "none";
//   }

//   // Remove the background color from all tabs
//   tabs = document.getElementsByClassName("tab");
//   for (i = 0; i < tabs.length; i++) {
//     tabs[i].style.backgroundColor = "";
//   }

//   // Fetch the content of the corresponding HTML file
//   const response = await fetch(`${tabName}.html`);
//   const content = await response.text();

//   // Show the current tab and set its background color
//   const tab = document.getElementById(tabName);
//   tab.innerHTML = content;
//   tab.style.display = "block";
//   evt.currentTarget.style.backgroundColor = "#ddd";
// }


$(document).ready(function() {
  // Load the content of the first tab by default
  $("#practicalProblems").load("hardware.html");

  // When a tab is clicked, load its content
  $(".nav-link").click(function() {
    var tabId = $(this).attr("href");
    $(tabId).load(tabId.substring(1) + ".html");
  });
});
