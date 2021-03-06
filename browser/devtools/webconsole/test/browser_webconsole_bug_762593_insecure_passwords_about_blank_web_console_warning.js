/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */
/*
 * Tests that errors about insecure passwords are logged
 * to the web console
 */

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-bug-762593-insecure-passwords-about-blank-web-console-warning.html";
const INSECURE_PASSWORD_MSG = "Password fields present on an insecure (http://) page. This is a security risk that allows user login credentials to be stolen.";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad(aEvent) {
    browser.removeEventListener(aEvent.type, onLoad, true);
    openConsole(null, function testInsecurePasswordErrorLogged (hud) {
      waitForMessages({
        webconsole: hud,
        messages: [
          {
            name: "Insecure password error displayed successfully",
            text: INSECURE_PASSWORD_MSG,
            category: CATEGORY_SECURITY,
            severity: SEVERITY_WARNING
          },
        ],
      }).then(finishTest);
    });
  }, true);
}
