$(document).ready(function() {
  console.log("Custom script executed."); // Add this line
  // Load the content of the first tab by default
  $("#problemDescription").load("problemDescription.html");

  // When a tab is clicked, load its content
  $(".nav-link").click(function() {
    var tabId = $(this).attr("href");
    $(tabId).load(tabId.substring(1) + ".html");
  });
});
