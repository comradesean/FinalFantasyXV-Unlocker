(function () {
    function showAlert(event) {
        if (event.preventDefault) event.preventDefault(); // Standard method
        else event.returnValue = false; // IE8 fallback

        alert("This page is an emulation designed to replicate an experience. Account functionality does not exist.");
    }

    function addEventListenerCompat(element, event, handler) {
        if (element.addEventListener) {
            element.addEventListener(event, handler, false);
        } else if (element.attachEvent) {
            element.attachEvent("on" + event, handler);
        }
    }

    var alertResetLink = document.getElementById("reset");
    var alertSignupLink = document.getElementById("signup_tab");

    if (alertResetLink) {
        addEventListenerCompat(alertResetLink, "click", showAlert);
    }
    if (alertSignupLink) {
        addEventListenerCompat(alertSignupLink, "click", showAlert);
    }
})();
