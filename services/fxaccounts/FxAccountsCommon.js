/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

const { interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Log.jsm");

// loglevel should be one of "Fatal", "Error", "Warn", "Info", "Config",
// "Debug", "Trace" or "All". If none is specified, "Debug" will be used by
// default.  Note "Debug" is usually appropriate so that when this log is
// included in the Sync file logs we get verbose output.
const PREF_LOG_LEVEL = "identity.fxaccounts.loglevel";
// The level of messages that will be dumped to the console.  If not specified,
// "Error" will be used.
const PREF_LOG_LEVEL_DUMP = "identity.fxaccounts.log.appender.dump";

// A pref that can be set so "sensitive" information (eg, personally
// identifiable info, credentials, etc) will be logged.
const PREF_LOG_SENSITIVE_DETAILS = "identity.fxaccounts.log.sensitive";

let exports = Object.create(null);

XPCOMUtils.defineLazyGetter(exports, 'log', function() {
  let log = Log.repository.getLogger("FirefoxAccounts");
  // We set the log level to debug, but the default dump appender is set to
  // the level reflected in the pref.  Other code that consumes FxA may then
  // choose to add another appender at a different level.
  log.level = Log.Level.Debug;
  let appender = new Log.DumpAppender();
  appender.level = Log.Level.Error;

  log.addAppender(appender);
  try {
    // The log itself.
    let level =
      Services.prefs.getPrefType(PREF_LOG_LEVEL) == Ci.nsIPrefBranch.PREF_STRING
      && Services.prefs.getCharPref(PREF_LOG_LEVEL);
    log.level = Log.Level[level] || Log.Level.Debug;

    // The appender.
    level =
      Services.prefs.getPrefType(PREF_LOG_LEVEL_DUMP) == Ci.nsIPrefBranch.PREF_STRING
      && Services.prefs.getCharPref(PREF_LOG_LEVEL_DUMP);
    appender.level = Log.Level[level] || Log.Level.Error;
  } catch (e) {
    log.error(e);
  }

  return log;
});

// A boolean to indicate if personally identifiable information (or anything
// else sensitive, such as credentials) should be logged.
XPCOMUtils.defineLazyGetter(exports, 'logPII', function() {
  try {
    return Services.prefs.getBoolPref(PREF_LOG_SENSITIVE_DETAILS);
  } catch (_) {
    return false;
  }
});

exports.FXACCOUNTS_PERMISSION = "firefox-accounts";

exports.DATA_FORMAT_VERSION = 1;
exports.DEFAULT_STORAGE_FILENAME = "signedInUser.json";

// Token life times.
// Having this parameter be short has limited security value and can cause
// spurious authentication values if the client's clock is skewed and
// we fail to adjust. See Bug 983256.
exports.ASSERTION_LIFETIME = 1000 * 3600 * 24 * 365 * 25; // 25 years
// This is a time period we want to guarantee that the assertion will be
// valid after we generate it (e.g., the signed cert won't expire in this
// period).
exports.ASSERTION_USE_PERIOD = 1000 * 60 * 5; // 5 minutes
exports.CERT_LIFETIME      = 1000 * 3600 * 6;  // 6 hours
exports.KEY_LIFETIME       = 1000 * 3600 * 12; // 12 hours

// Polling timings.
exports.POLL_SESSION       = 1000 * 60 * 5;    // 5 minutes
exports.POLL_STEP          = 1000 * 3;         // 3 seconds

// Observer notifications.
exports.ONLOGIN_NOTIFICATION = "fxaccounts:onlogin";
exports.ONVERIFIED_NOTIFICATION = "fxaccounts:onverified";
exports.ONLOGOUT_NOTIFICATION = "fxaccounts:onlogout";
// Internal to services/fxaccounts only
exports.ON_FXA_UPDATE_NOTIFICATION = "fxaccounts:update";

// UI Requests.
exports.UI_REQUEST_SIGN_IN_FLOW = "signInFlow";
exports.UI_REQUEST_REFRESH_AUTH = "refreshAuthentication";

// Server errno.
// From https://github.com/mozilla/fxa-auth-server/blob/master/docs/api.md#response-format
exports.ERRNO_ACCOUNT_ALREADY_EXISTS         = 101;
exports.ERRNO_ACCOUNT_DOES_NOT_EXIST         = 102;
exports.ERRNO_INCORRECT_PASSWORD             = 103;
exports.ERRNO_UNVERIFIED_ACCOUNT             = 104;
exports.ERRNO_INVALID_VERIFICATION_CODE      = 105;
exports.ERRNO_NOT_VALID_JSON_BODY            = 106;
exports.ERRNO_INVALID_BODY_PARAMETERS        = 107;
exports.ERRNO_MISSING_BODY_PARAMETERS        = 108;
exports.ERRNO_INVALID_REQUEST_SIGNATURE      = 109;
exports.ERRNO_INVALID_AUTH_TOKEN             = 110;
exports.ERRNO_INVALID_AUTH_TIMESTAMP         = 111;
exports.ERRNO_MISSING_CONTENT_LENGTH         = 112;
exports.ERRNO_REQUEST_BODY_TOO_LARGE         = 113;
exports.ERRNO_TOO_MANY_CLIENT_REQUESTS       = 114;
exports.ERRNO_INVALID_AUTH_NONCE             = 115;
exports.ERRNO_ENDPOINT_NO_LONGER_SUPPORTED   = 116;
exports.ERRNO_INCORRECT_LOGIN_METHOD         = 117;
exports.ERRNO_INCORRECT_KEY_RETRIEVAL_METHOD = 118;
exports.ERRNO_INCORRECT_API_VERSION          = 119;
exports.ERRNO_INCORRECT_EMAIL_CASE           = 120;
exports.ERRNO_SERVICE_TEMP_UNAVAILABLE       = 201;
exports.ERRNO_UNKNOWN_ERROR                  = 999;

// Errors.
exports.ERROR_ACCOUNT_ALREADY_EXISTS         = "ACCOUNT_ALREADY_EXISTS";
exports.ERROR_ACCOUNT_DOES_NOT_EXIST         = "ACCOUNT_DOES_NOT_EXIST ";
exports.ERROR_ALREADY_SIGNED_IN_USER         = "ALREADY_SIGNED_IN_USER";
exports.ERROR_ENDPOINT_NO_LONGER_SUPPORTED   = "ENDPOINT_NO_LONGER_SUPPORTED";
exports.ERROR_INCORRECT_API_VERSION          = "INCORRECT_API_VERSION";
exports.ERROR_INCORRECT_EMAIL_CASE           = "INCORRECT_EMAIL_CASE";
exports.ERROR_INCORRECT_KEY_RETRIEVAL_METHOD = "INCORRECT_KEY_RETRIEVAL_METHOD";
exports.ERROR_INCORRECT_LOGIN_METHOD         = "INCORRECT_LOGIN_METHOD";
exports.ERROR_INVALID_EMAIL                  = "INVALID_EMAIL";
exports.ERROR_INVALID_AUDIENCE               = "INVALID_AUDIENCE";
exports.ERROR_INVALID_AUTH_TOKEN             = "INVALID_AUTH_TOKEN";
exports.ERROR_INVALID_AUTH_TIMESTAMP         = "INVALID_AUTH_TIMESTAMP";
exports.ERROR_INVALID_AUTH_NONCE             = "INVALID_AUTH_NONCE";
exports.ERROR_INVALID_BODY_PARAMETERS        = "INVALID_BODY_PARAMETERS";
exports.ERROR_INVALID_PASSWORD               = "INVALID_PASSWORD";
exports.ERROR_INVALID_VERIFICATION_CODE      = "INVALID_VERIFICATION_CODE";
exports.ERROR_INVALID_REFRESH_AUTH_VALUE     = "INVALID_REFRESH_AUTH_VALUE";
exports.ERROR_INVALID_REQUEST_SIGNATURE      = "INVALID_REQUEST_SIGNATURE";
exports.ERROR_INTERNAL_INVALID_USER          = "INTERNAL_ERROR_INVALID_USER";
exports.ERROR_MISSING_BODY_PARAMETERS        = "MISSING_BODY_PARAMETERS";
exports.ERROR_MISSING_CONTENT_LENGTH         = "MISSING_CONTENT_LENGTH";
exports.ERROR_NO_TOKEN_SESSION               = "NO_TOKEN_SESSION";
exports.ERROR_NO_SILENT_REFRESH_AUTH         = "NO_SILENT_REFRESH_AUTH";
exports.ERROR_NOT_VALID_JSON_BODY            = "NOT_VALID_JSON_BODY";
exports.ERROR_OFFLINE                        = "OFFLINE";
exports.ERROR_PERMISSION_DENIED              = "PERMISSION_DENIED";
exports.ERROR_REQUEST_BODY_TOO_LARGE         = "REQUEST_BODY_TOO_LARGE";
exports.ERROR_SERVER_ERROR                   = "SERVER_ERROR";
exports.ERROR_TOO_MANY_CLIENT_REQUESTS       = "TOO_MANY_CLIENT_REQUESTS";
exports.ERROR_SERVICE_TEMP_UNAVAILABLE       = "SERVICE_TEMPORARY_UNAVAILABLE";
exports.ERROR_UI_ERROR                       = "UI_ERROR";
exports.ERROR_UI_REQUEST                     = "UI_REQUEST";
exports.ERROR_UNKNOWN                        = "UNKNOWN_ERROR";
exports.ERROR_UNVERIFIED_ACCOUNT             = "UNVERIFIED_ACCOUNT";

// FxAccounts has the ability to "split" the credentials between a plain-text
// JSON file in the profile dir and in the login manager.
// These constants relate to that.

// The fields we save in the plaintext JSON.
// See bug 1013064 comments 23-25 for why the sessionToken is "safe"
exports.FXA_PWDMGR_PLAINTEXT_FIELDS = ["email", "verified", "authAt",
                                       "sessionToken", "uid"];
// The pseudo-host we use in the login manager
exports.FXA_PWDMGR_HOST = "chrome://FirefoxAccounts";
// The realm we use in the login manager.
exports.FXA_PWDMGR_REALM = "Firefox Accounts credentials";

// Error matching.
exports.SERVER_ERRNO_TO_ERROR = {};

for (let id in exports) {
  this[id] = exports[id];
}

// Allow this file to be imported via Components.utils.import().
this.EXPORTED_SYMBOLS = Object.keys(exports);

// Set these up now that everything has been loaded into |this|.
SERVER_ERRNO_TO_ERROR[ERRNO_ACCOUNT_ALREADY_EXISTS]         = ERROR_ACCOUNT_ALREADY_EXISTS;
SERVER_ERRNO_TO_ERROR[ERRNO_ACCOUNT_DOES_NOT_EXIST]         = ERROR_ACCOUNT_DOES_NOT_EXIST;
SERVER_ERRNO_TO_ERROR[ERRNO_INCORRECT_PASSWORD]             = ERROR_INVALID_PASSWORD;
SERVER_ERRNO_TO_ERROR[ERRNO_UNVERIFIED_ACCOUNT]             = ERROR_UNVERIFIED_ACCOUNT;
SERVER_ERRNO_TO_ERROR[ERRNO_INVALID_VERIFICATION_CODE]      = ERROR_INVALID_VERIFICATION_CODE;
SERVER_ERRNO_TO_ERROR[ERRNO_NOT_VALID_JSON_BODY]            = ERROR_NOT_VALID_JSON_BODY;
SERVER_ERRNO_TO_ERROR[ERRNO_INVALID_BODY_PARAMETERS]        = ERROR_INVALID_BODY_PARAMETERS;
SERVER_ERRNO_TO_ERROR[ERRNO_MISSING_BODY_PARAMETERS]        = ERROR_MISSING_BODY_PARAMETERS;
SERVER_ERRNO_TO_ERROR[ERRNO_INVALID_REQUEST_SIGNATURE]      = ERROR_INVALID_REQUEST_SIGNATURE;
SERVER_ERRNO_TO_ERROR[ERRNO_INVALID_AUTH_TOKEN]             = ERROR_INVALID_AUTH_TOKEN;
SERVER_ERRNO_TO_ERROR[ERRNO_INVALID_AUTH_TIMESTAMP]         = ERROR_INVALID_AUTH_TIMESTAMP;
SERVER_ERRNO_TO_ERROR[ERRNO_MISSING_CONTENT_LENGTH]         = ERROR_MISSING_CONTENT_LENGTH;
SERVER_ERRNO_TO_ERROR[ERRNO_REQUEST_BODY_TOO_LARGE]         = ERROR_REQUEST_BODY_TOO_LARGE;
SERVER_ERRNO_TO_ERROR[ERRNO_TOO_MANY_CLIENT_REQUESTS]       = ERROR_TOO_MANY_CLIENT_REQUESTS;
SERVER_ERRNO_TO_ERROR[ERRNO_INVALID_AUTH_NONCE]             = ERROR_INVALID_AUTH_NONCE;
SERVER_ERRNO_TO_ERROR[ERRNO_ENDPOINT_NO_LONGER_SUPPORTED]   = ERROR_ENDPOINT_NO_LONGER_SUPPORTED;
SERVER_ERRNO_TO_ERROR[ERRNO_INCORRECT_LOGIN_METHOD]         = ERROR_INCORRECT_LOGIN_METHOD;
SERVER_ERRNO_TO_ERROR[ERRNO_INCORRECT_KEY_RETRIEVAL_METHOD] = ERROR_INCORRECT_KEY_RETRIEVAL_METHOD;
SERVER_ERRNO_TO_ERROR[ERRNO_INCORRECT_API_VERSION]          = ERROR_INCORRECT_API_VERSION;
SERVER_ERRNO_TO_ERROR[ERRNO_INCORRECT_EMAIL_CASE]           = ERROR_INCORRECT_EMAIL_CASE;
SERVER_ERRNO_TO_ERROR[ERRNO_SERVICE_TEMP_UNAVAILABLE]       = ERROR_SERVICE_TEMP_UNAVAILABLE;
SERVER_ERRNO_TO_ERROR[ERRNO_UNKNOWN_ERROR]                  = ERROR_UNKNOWN;
