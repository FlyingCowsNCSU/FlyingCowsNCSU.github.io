$(document).ready(function() {
  // Load the content of the first tab by default
  $("#practicalProblems").load("practicalProblems.html");

  // When a tab is clicked, load its content
  $(".nav-link").click(function() {
    var tabId = $(this).attr("href");
    $(tabId).load(tabId.substring(1) + ".html");
  });
});
