(function () {
    function getQueryParam(param) {
        var queryString = window.location.search.substring(1);
        var params = queryString.split("&");
        var i, pair;

        for (i = 0; i < params.length; i++) {
            pair = params[i].split("=");
            if (decodeURIComponent && decodeURIComponent(pair[0]) === param) {
                return decodeURIComponent(pair[1] || "");
            } else if (pair[0] === param) {
                return pair[1] || "";
            }
        }
        return "";
    }

    function signIn() {
        // Get parameters manually
        var redirectUri = getQueryParam("redirect_uri") || "http://localhost/";
        var scope = getQueryParam("scope") || "user_read+openid";
        var state = getQueryParam("state") || "a1a6c438262f44f1b97127ac368dbfdf";

        // Mock tokens (Replace with real token retrieval if necessary)
        var accessToken = "aa4zpynxujgekwgh36we1z4xzzwlr5";
        var idToken = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6IjEifQ...";

        // Construct redirect URL with hash fragment instead of query parameters
        var redirectUrl = redirectUri + "#access_token=" + accessToken +
            "&id_token=" + idToken +
            "&scope=" + scope +
            "&state=" + state +
            "&token_type=bearer";

        // Redirect (use window.location.href for IE8 compatibility)
        if (window.location.replace) {
            window.location.replace(redirectUrl);
        } else {
            window.location.href = redirectUrl;
        }
    }

    function addEventListenerCompat(element, event, handler) {
        if (element.addEventListener) {
            element.addEventListener(event, handler, false);
        } else if (element.attachEvent) {
            element.attachEvent("on" + event, handler);
        }
    }

    function getLoginButton() {
        // Use getElementsByClassName for IE8 compatibility
        if (document.getElementsByClassName) {
            var buttons = document.getElementsByClassName("js-login-button");
            return buttons.length > 0 ? buttons[0] : null;
        } else {
            // Fallback for really old browsers (requires specific ID)
            return document.getElementById("login-button") || null;
        }
    }

    function setupLoginButton() {
        var loginButton = getLoginButton();
        if (loginButton) {
            addEventListenerCompat(loginButton, "click", function (event) {
                if (event && event.preventDefault) {
                    event.preventDefault(); // Modern browsers
                } else {
                    event.returnValue = false; // IE8 fallback
                }
                signIn();
            });
        }
    }

    // Run when page loads
    if (document.readyState === "complete") {
        setupLoginButton();
    } else {
        addEventListenerCompat(window, "load", setupLoginButton);
    }
})();