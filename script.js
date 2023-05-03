$(document).ready(function() {
  console.log("Custom script executed.");

  // Load the content of the active tab by default
  var activeTabId = $(".nav-link.active").attr("href");
  $(activeTabId).load(activeTabId.substring(1) + ".html");

  // When a tab is clicked, load its content
  $(".nav-link").click(function() {
    var tabId = $(this).attr("href");
    $(tabId).load(tabId.substring(1) + ".html");
  });
});
