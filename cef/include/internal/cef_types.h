// Copyright (c) 2014 Marshall A. Greenblatt. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the name Chromium Embedded
// Framework nor the names of its contributors may be used to endorse
// or promote products derived from this software without specific prior
// written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#ifndef CEF_INCLUDE_INTERNAL_CEF_TYPES_H_
#define CEF_INCLUDE_INTERNAL_CEF_TYPES_H_
#pragma once

#include "include/base/cef_build.h"
#include "include/internal/cef_string.h"
#include "include/internal/cef_string_list.h"
#include "include/internal/cef_time.h"

// Bring in platform-specific definitions.
#if defined(OS_WIN)
#include "include/internal/cef_types_win.h"
#elif defined(OS_MACOSX)
#include "include/internal/cef_types_mac.h"
#elif defined(OS_LINUX)
#include "include/internal/cef_types_linux.h"
#endif

#include <limits.h>         // For UINT_MAX
#include <stddef.h>         // For size_t

// The NSPR system headers define 64-bit as |long| when possible, except on
// Mac OS X.  In order to not have typedef mismatches, we do the same on LP64.
//
// On Mac OS X, |long long| is used for 64-bit types for compatibility with
// <inttypes.h> format macros even in the LP64 model.
#if defined(__LP64__) && !defined(OS_MACOSX) && !defined(OS_OPENBSD)
typedef long                int64;  // NOLINT(runtime/int)
typedef unsigned long       uint64;  // NOLINT(runtime/int)
#else
typedef long long           int64;  // NOLINT(runtime/int)
typedef unsigned long long  uint64;  // NOLINT(runtime/int)
#endif

// TODO: Remove these type guards.  These are to avoid conflicts with
// obsolete/protypes.h in the Gecko SDK.
#ifndef _INT32
#define _INT32
typedef int                 int32;
#endif

// TODO: Remove these type guards.  These are to avoid conflicts with
// obsolete/protypes.h in the Gecko SDK.
#ifndef _UINT32
#define _UINT32
typedef unsigned int       uint32;
#endif

// UTF-16 character type
#ifndef char16
#if defined(WIN32)
typedef wchar_t             char16;
#else
typedef unsigned short      char16;
#endif
#endif

// 32-bit ARGB color value, not premultiplied. The color components are always
// in a known order. Equivalent to the SkColor type.
typedef uint32              cef_color_t;

// Return the alpha byte from a cef_color_t value.
#define CefColorGetA(color)      (((color) >> 24) & 0xFF)
// Return the red byte from a cef_color_t value.
#define CefColorGetR(color)      (((color) >> 16) & 0xFF)
// Return the green byte from a cef_color_t value.
#define CefColorGetG(color)      (((color) >>  8) & 0xFF)
// Return the blue byte from a cef_color_t value.
#define CefColorGetB(color)      (((color) >>  0) & 0xFF)

// Return an cef_color_t value with the specified byte component values.
#define CefColorSetARGB(a, r, g, b) \
    static_cast<cef_color_t>( \
        (static_cast<unsigned>(a) << 24) | \
        (static_cast<unsigned>(r) << 16) | \
        (static_cast<unsigned>(g) << 8) | \
        (static_cast<unsigned>(b) << 0))

// Return an int64 value with the specified low and high int32 component values.
#define CefInt64Set(int32_low, int32_high) \
    static_cast<int64>((static_cast<uint32>(int32_low)) | \
        (static_cast<int64>(static_cast<int32>(int32_high))) << 32)

// Return the low int32 value from an int64 value.
#define CefInt64GetLow(int64_val) static_cast<int32>(int64_val)
// Return the high int32 value from an int64 value.
#define CefInt64GetHigh(int64_val) \
    static_cast<int32>((static_cast<int64>(int64_val) >> 32) & 0xFFFFFFFFL)


#ifdef __cplusplus
extern "C" {
#endif

///
// Log severity levels.
///
typedef enum {
  ///
  // Default logging (currently INFO logging).
  ///
  LOGSEVERITY_DEFAULT,

  ///
  // Verbose logging.
  ///
  LOGSEVERITY_VERBOSE,

  ///
  // INFO logging.
  ///
  LOGSEVERITY_INFO,

  ///
  // WARNING logging.
  ///
  LOGSEVERITY_WARNING,

  ///
  // ERROR logging.
  ///
  LOGSEVERITY_ERROR,

  ///
  // Completely disable logging.
  ///
  LOGSEVERITY_DISABLE = 99
} cef_log_severity_t;

///
// Represents the state of a setting.
///
typedef enum {
  ///
  // Use the default state for the setting.
  ///
  STATE_DEFAULT = 0,

  ///
  // Enable or allow the setting.
  ///
  STATE_ENABLED,

  ///
  // Disable or disallow the setting.
  ///
  STATE_DISABLED,
} cef_state_t;

///
// Initialization settings. Specify NULL or 0 to get the recommended default
// values. Many of these and other settings can also configured using command-
// line switches.
///
typedef struct _cef_settings_t {
  ///
  // Size of this structure.
  ///
  size_t size;

  ///
  // Set to true (1) to use a single process for the browser and renderer. This
  // run mode is not officially supported by Chromium and is less stable than
  // the multi-process default. Also configurable using the "single-process"
  // command-line switch.
  ///
  int single_process;

  ///
  // Set to true (1) to disable the sandbox for sub-processes. See
  // cef_sandbox_win.h for requirements to enable the sandbox on Windows. Also
  // configurable using the "no-sandbox" command-line switch.
  ///
  int no_sandbox;

  ///
  // The path to a separate executable that will be launched for sub-processes.
  // By default the browser process executable is used. See the comments on
  // CefExecuteProcess() for details. Also configurable using the
  // "browser-subprocess-path" command-line switch.
  ///
  cef_string_t browser_subprocess_path;

  ///
  // Set to true (1) to have the browser process message loop run in a separate
  // thread. If false (0) than the CefDoMessageLoopWork() function must be
  // called from your application message loop.
  ///
  int multi_threaded_message_loop;

  ///
  // Set to true (1) to enable windowless (off-screen) rendering support. Do not
  // enable this value if the application does not use windowless rendering as
  // it may reduce rendering performance on some systems.
  ///
  int windowless_rendering_enabled;

  ///
  // Set to true (1) to disable configuration of browser process features using
  // standard CEF and Chromium command-line arguments. Configuration can still
  // be specified using CEF data structures or via the
  // CefApp::OnBeforeCommandLineProcessing() method.
  ///
  int command_line_args_disabled;

  ///
  // The location where cache data will be stored on disk. If empty an in-memory
  // cache will be used for some features and a temporary disk cache for others.
  // HTML5 databases such as localStorage will only persist across sessions if a
  // cache path is specified.
  ///
  cef_string_t cache_path;

  ///
  // To persist session cookies (cookies without an expiry date or validity
  // interval) by default when using the global cookie manager set this value to
  // true. Session cookies are generally intended to be transient and most Web
  // browsers do not persist them. A |cache_path| value must also be specified to
  // enable this feature. Also configurable using the "persist-session-cookies"
  // command-line switch.
  ///
  int persist_session_cookies;

  ///
  // Value that will be returned as the User-Agent HTTP header. If empty the
  // default User-Agent string will be used. Also configurable using the
  // "user-agent" command-line switch.
  ///
  cef_string_t user_agent;

  ///
  // Value that will be inserted as the product portion of the default
  // User-Agent string. If empty the Chromium product version will be used. If
  // |userAgent| is specified this value will be ignored. Also configurable
  // using the "product-version" command-line switch.
  ///
  cef_string_t product_version;

  ///
  // The locale string that will be passed to WebKit. If empty the default
  // locale of "en-US" will be used. This value is ignored on Linux where locale
  // is determined using environment variable parsing with the precedence order:
  // LANGUAGE, LC_ALL, LC_MESSAGES and LANG. Also configurable using the "lang"
  // command-line switch.
  ///
  cef_string_t locale;

  ///
  // The directory and file name to use for the debug log. If empty, the
  // default name of "debug.log" will be used and the file will be written
  // to the application directory. Also configurable using the "log-file"
  // command-line switch.
  ///
  cef_string_t log_file;

  ///
  // The log severity. Only messages of this severity level or higher will be
  // logged. Also configurable using the "log-severity" command-line switch with
  // a value of "verbose", "info", "warning", "error", "error-report" or
  // "disable".
  ///
  cef_log_severity_t log_severity;

  ///
  // Custom flags that will be used when initializing the V8 JavaScript engine.
  // The consequences of using custom flags may not be well tested. Also
  // configurable using the "js-flags" command-line switch.
  ///
  cef_string_t javascript_flags;

  ///
  // The fully qualified path for the resources directory. If this value is
  // empty the cef.pak and/or devtools_resources.pak files must be located in
  // the module directory on Windows/Linux or the app bundle Resources directory
  // on Mac OS X. Also configurable using the "resources-dir-path" command-line
  // switch.
  ///
  cef_string_t resources_dir_path;

  ///
  // The fully qualified path for the locales directory. If this value is empty
  // the locales directory must be located in the module directory. This value
  // is ignored on Mac OS X where pack files are always loaded from the app
  // bundle Resources directory. Also configurable using the "locales-dir-path"
  // command-line switch.
  ///
  cef_string_t locales_dir_path;

  ///
  // Set to true (1) to disable loading of pack files for resources and locales.
  // A resource bundle handler must be provided for the browser and render
  // processes via CefApp::GetResourceBundleHandler() if loading of pack files
  // is disabled. Also configurable using the "disable-pack-loading" command-
  // line switch.
  ///
  int pack_loading_disabled;

  ///
  // Set to a value between 1024 and 65535 to enable remote debugging on the
  // specified port. For example, if 8080 is specified the remote debugging URL
  // will be http://localhost:8080. CEF can be remotely debugged from any CEF or
  // Chrome browser window. Also configurable using the "remote-debugging-port"
  // command-line switch.
  ///
  int remote_debugging_port;

  ///
  // The number of stack trace frames to capture for uncaught exceptions.
  // Specify a positive value to enable the CefV8ContextHandler::
  // OnUncaughtException() callback. Specify 0 (default value) and
  // OnUncaughtException() will not be called. Also configurable using the
  // "uncaught-exception-stack-size" command-line switch.
  ///
  int uncaught_exception_stack_size;

  ///
  // By default CEF V8 references will be invalidated (the IsValid() method will
  // return false) after the owning context has been released. This reduces the
  // need for external record keeping and avoids crashes due to the use of V8
  // references after the associated context has been released.
  //
  // CEF currently offers two context safety implementations with different
  // performance characteristics. The default implementation (value of 0) uses a
  // map of hash values and should provide better performance in situations with
  // a small number contexts. The alternate implementation (value of 1) uses a
  // hidden value attached to each context and should provide better performance
  // in situations with a large number of contexts.
  //
  // If you need better performance in the creation of V8 references and you
  // plan to manually track context lifespan you can disable context safety by
  // specifying a value of -1.
  //
  // Also configurable using the "context-safety-implementation" command-line
  // switch.
  ///
  int context_safety_implementation;

  ///
  // Set to true (1) to ignore errors related to invalid SSL certificates.
  // Enabling this setting can lead to potential security vulnerabilities like
  // "man in the middle" attacks. Applications that load content from the
  // internet should not enable this setting. Also configurable using the
  // "ignore-certificate-errors" command-line switch.
  ///
  int ignore_certificate_errors;

  ///
  // Opaque background color used for accelerated content. By default the
  // background color will be white. Only the RGB compontents of the specified
  // value will be used. The alpha component must greater than 0 to enable use
  // of the background color but will be otherwise ignored.
  ///
  cef_color_t background_color;
} cef_settings_t;

///
// Browser initialization settings. Specify NULL or 0 to get the recommended
// default values. The consequences of using custom values may not be well
// tested. Many of these and other settings can also configured using command-
// line switches.
///
typedef struct _cef_browser_settings_t {
  ///
  // Size of this structure.
  ///
  size_t size;

  ///
  // The maximum rate in frames per second (fps) that CefRenderHandler::OnPaint
  // will be called for a windowless browser. The actual fps may be lower if
  // the browser cannot generate frames at the requested rate. The minimum
  // value is 1 and the maximum value is 60 (default 30).
  ///
  int windowless_frame_rate;

  // The below values map to WebPreferences settings.

  ///
  // Font settings.
  ///
  cef_string_t standard_font_family;
  cef_string_t fixed_font_family;
  cef_string_t serif_font_family;
  cef_string_t sans_serif_font_family;
  cef_string_t cursive_font_family;
  cef_string_t fantasy_font_family;
  int default_font_size;
  int default_fixed_font_size;
  int minimum_font_size;
  int minimum_logical_font_size;

  ///
  // Default encoding for Web content. If empty "ISO-8859-1" will be used. Also
  // configurable using the "default-encoding" command-line switch.
  ///
  cef_string_t default_encoding;

  ///
  // Controls the loading of fonts from remote sources. Also configurable using
  // the "disable-remote-fonts" command-line switch.
  ///
  cef_state_t remote_fonts;

  ///
  // Controls whether JavaScript can be executed. Also configurable using the
  // "disable-javascript" command-line switch.
  ///
  cef_state_t javascript;

  ///
  // Controls whether JavaScript can be used for opening windows. Also
  // configurable using the "disable-javascript-open-windows" command-line
  // switch.
  ///
  cef_state_t javascript_open_windows;

  ///
  // Controls whether JavaScript can be used to close windows that were not
  // opened via JavaScript. JavaScript can still be used to close windows that
  // were opened via JavaScript. Also configurable using the
  // "disable-javascript-close-windows" command-line switch.
  ///
  cef_state_t javascript_close_windows;

  ///
  // Controls whether JavaScript can access the clipboard. Also configurable
  // using the "disable-javascript-access-clipboard" command-line switch.
  ///
  cef_state_t javascript_access_clipboard;

  ///
  // Controls whether DOM pasting is supported in the editor via
  // execCommand("paste"). The |javascript_access_clipboard| setting must also
  // be enabled. Also configurable using the "disable-javascript-dom-paste"
  // command-line switch.
  ///
  cef_state_t javascript_dom_paste;

  ///
  // Controls whether the caret position will be drawn. Also configurable using
  // the "enable-caret-browsing" command-line switch.
  ///
  cef_state_t caret_browsing;

  ///
  // Controls whether the Java plugin will be loaded. Also configurable using
  // the "disable-java" command-line switch.
  ///
  cef_state_t java;

  ///
  // Controls whether any plugins will be loaded. Also configurable using the
  // "disable-plugins" command-line switch.
  ///
  cef_state_t plugins;

  ///
  // Controls whether file URLs will have access to all URLs. Also configurable
  // using the "allow-universal-access-from-files" command-line switch.
  ///
  cef_state_t universal_access_from_file_urls;

  ///
  // Controls whether file URLs will have access to other file URLs. Also
  // configurable using the "allow-access-from-files" command-line switch.
  ///
  cef_state_t file_access_from_file_urls;

  ///
  // Controls whether web security restrictions (same-origin policy) will be
  // enforced. Disabling this setting is not recommend as it will allow risky
  // security behavior such as cross-site scripting (XSS). Also configurable
  // using the "disable-web-security" command-line switch.
  ///
  cef_state_t web_security;

  ///
  // Controls whether image URLs will be loaded from the network. A cached image
  // will still be rendered if requested. Also configurable using the
  // "disable-image-loading" command-line switch.
  ///
  cef_state_t image_loading;

  ///
  // Controls whether standalone images will be shrunk to fit the page. Also
  // configurable using the "image-shrink-standalone-to-fit" command-line
  // switch.
  ///
  cef_state_t image_shrink_standalone_to_fit;

  ///
  // Controls whether text areas can be resized. Also configurable using the
  // "disable-text-area-resize" command-line switch.
  ///
  cef_state_t text_area_resize;

  ///
  // Controls whether the tab key can advance focus to links. Also configurable
  // using the "disable-tab-to-links" command-line switch.
  ///
  cef_state_t tab_to_links;

  ///
  // Controls whether local storage can be used. Also configurable using the
  // "disable-local-storage" command-line switch.
  ///
  cef_state_t local_storage;

  ///
  // Controls whether databases can be used. Also configurable using the
  // "disable-databases" command-line switch.
  ///
  cef_state_t databases;

  ///
  // Controls whether the application cache can be used. Also configurable using
  // the "disable-application-cache" command-line switch.
  ///
  cef_state_t application_cache;

  ///
  // Controls whether WebGL can be used. Note that WebGL requires hardware
  // support and may not work on all systems even when enabled. Also
  // configurable using the "disable-webgl" command-line switch.
  ///
  cef_state_t webgl;

  ///
  // Opaque background color used for the browser before a document is loaded
  // and when no document color is specified. By default the background color
  // will be the same as CefSettings.background_color. Only the RGB compontents
  // of the specified value will be used. The alpha component must greater than
  // 0 to enable use of the background color but will be otherwise ignored.
  ///
  cef_color_t background_color;
} cef_browser_settings_t;

///
// URL component parts.
///
typedef struct _cef_urlparts_t {
  ///
  // The complete URL specification.
  ///
  cef_string_t spec;

  ///
  // Scheme component not including the colon (e.g., "http").
  ///
  cef_string_t scheme;

  ///
  // User name component.
  ///
  cef_string_t username;

  ///
  // Password component.
  ///
  cef_string_t password;

  ///
  // Host component. This may be a hostname, an IPv4 address or an IPv6 literal
  // surrounded by square brackets (e.g., "[2001:db8::1]").
  ///
  cef_string_t host;

  ///
  // Port number component.
  ///
  cef_string_t port;

  ///
  // Origin contains just the scheme, host, and port from a URL. Equivalent to
  // clearing any username and password, replacing the path with a slash, and
  // clearing everything after that. This value will be empty for non-standard
  // URLs.
  ///
  cef_string_t origin;

  ///
  // Path component including the first slash following the host.
  ///
  cef_string_t path;

  ///
  // Query string component (i.e., everything following the '?').
  ///
  cef_string_t query;
} cef_urlparts_t;

///
// Cookie information.
///
typedef struct _cef_cookie_t {
  ///
  // The cookie name.
  ///
  cef_string_t name;

  ///
  // The cookie value.
  ///
  cef_string_t value;

  ///
  // If |domain| is empty a host cookie will be created instead of a domain
  // cookie. Domain cookies are stored with a leading "." and are visible to
  // sub-domains whereas host cookies are not.
  ///
  cef_string_t domain;

  ///
  // If |path| is non-empty only URLs at or below the path will get the cookie
  // value.
  ///
  cef_string_t path;

  ///
  // If |secure| is true the cookie will only be sent for HTTPS requests.
  ///
  int secure;

  ///
  // If |httponly| is true the cookie will only be sent for HTTP requests.
  ///
  int httponly;

  ///
  // The cookie creation date. This is automatically populated by the system on
  // cookie creation.
  ///
  cef_time_t creation;

  ///
  // The cookie last access date. This is automatically populated by the system
  // on access.
  ///
  cef_time_t last_access;

  ///
  // The cookie expiration date is only valid if |has_expires| is true.
  ///
  int has_expires;
  cef_time_t expires;
} cef_cookie_t;

///
// Process termination status values.
///
typedef enum {
  ///
  // Non-zero exit status.
  ///
  TS_ABNORMAL_TERMINATION,

  ///
  // SIGKILL or task manager kill.
  ///
  TS_PROCESS_WAS_KILLED,

  ///
  // Segmentation fault.
  ///
  TS_PROCESS_CRASHED,
} cef_termination_status_t;

///
// Path key values.
///
typedef enum {
  ///
  // Current directory.
  ///
  PK_DIR_CURRENT,

  ///
  // Directory containing PK_FILE_EXE.
  ///
  PK_DIR_EXE,

  ///
  // Directory containing PK_FILE_MODULE.
  ///
  PK_DIR_MODULE,

  ///
  // Temporary directory.
  ///
  PK_DIR_TEMP,

  ///
  // Path and filename of the current executable.
  ///
  PK_FILE_EXE,

  ///
  // Path and filename of the module containing the CEF code (usually the libcef
  // module).
  ///
  PK_FILE_MODULE,
} cef_path_key_t;

///
// Storage types.
///
typedef enum {
  ST_LOCALSTORAGE = 0,
  ST_SESSIONSTORAGE,
} cef_storage_type_t;

///
// Supported error code values. See net\base\net_error_list.h for complete
// descriptions of the error codes.
///
typedef enum {
  ERR_NONE = 0,
  ERR_FAILED = -2,
  ERR_ABORTED = -3,
  ERR_INVALID_ARGUMENT = -4,
  ERR_INVALID_HANDLE = -5,
  ERR_FILE_NOT_FOUND = -6,
  ERR_TIMED_OUT = -7,
  ERR_FILE_TOO_BIG = -8,
  ERR_UNEXPECTED = -9,
  ERR_ACCESS_DENIED = -10,
  ERR_NOT_IMPLEMENTED = -11,
  ERR_CONNECTION_CLOSED = -100,
  ERR_CONNECTION_RESET = -101,
  ERR_CONNECTION_REFUSED = -102,
  ERR_CONNECTION_ABORTED = -103,
  ERR_CONNECTION_FAILED = -104,
  ERR_NAME_NOT_RESOLVED = -105,
  ERR_INTERNET_DISCONNECTED = -106,
  ERR_SSL_PROTOCOL_ERROR = -107,
  ERR_ADDRESS_INVALID = -108,
  ERR_ADDRESS_UNREACHABLE = -109,
  ERR_SSL_CLIENT_AUTH_CERT_NEEDED = -110,
  ERR_TUNNEL_CONNECTION_FAILED = -111,
  ERR_NO_SSL_VERSIONS_ENABLED = -112,
  ERR_SSL_VERSION_OR_CIPHER_MISMATCH = -113,
  ERR_SSL_RENEGOTIATION_REQUESTED = -114,
  ERR_CERT_COMMON_NAME_INVALID = -200,
  ERR_CERT_DATE_INVALID = -201,
  ERR_CERT_AUTHORITY_INVALID = -202,
  ERR_CERT_CONTAINS_ERRORS = -203,
  ERR_CERT_NO_REVOCATION_MECHANISM = -204,
  ERR_CERT_UNABLE_TO_CHECK_REVOCATION = -205,
  ERR_CERT_REVOKED = -206,
  ERR_CERT_INVALID = -207,
  ERR_CERT_END = -208,
  ERR_INVALID_URL = -300,
  ERR_DISALLOWED_URL_SCHEME = -301,
  ERR_UNKNOWN_URL_SCHEME = -302,
  ERR_TOO_MANY_REDIRECTS = -310,
  ERR_UNSAFE_REDIRECT = -311,
  ERR_UNSAFE_PORT = -312,
  ERR_INVALID_RESPONSE = -320,
  ERR_INVALID_CHUNKED_ENCODING = -321,
  ERR_METHOD_NOT_SUPPORTED = -322,
  ERR_UNEXPECTED_PROXY_AUTH = -323,
  ERR_EMPTY_RESPONSE = -324,
  ERR_RESPONSE_HEADERS_TOO_BIG = -325,
  ERR_CACHE_MISS = -400,
  ERR_INSECURE_RESPONSE = -501,
} cef_errorcode_t;

///
// "Verb" of a drag-and-drop operation as negotiated between the source and
// destination. These constants match their equivalents in WebCore's
// DragActions.h and should not be renumbered.
///
typedef enum {
    DRAG_OPERATION_NONE    = 0,
    DRAG_OPERATION_COPY    = 1,
    DRAG_OPERATION_LINK    = 2,
    DRAG_OPERATION_GENERIC = 4,
    DRAG_OPERATION_PRIVATE = 8,
    DRAG_OPERATION_MOVE    = 16,
    DRAG_OPERATION_DELETE  = 32,
    DRAG_OPERATION_EVERY   = UINT_MAX
} cef_drag_operations_mask_t;

///
// V8 access control values.
///
typedef enum {
  V8_ACCESS_CONTROL_DEFAULT               = 0,
  V8_ACCESS_CONTROL_ALL_CAN_READ          = 1,
  V8_ACCESS_CONTROL_ALL_CAN_WRITE         = 1 << 1,
  V8_ACCESS_CONTROL_PROHIBITS_OVERWRITING = 1 << 2
} cef_v8_accesscontrol_t;

///
// V8 property attribute values.
///
typedef enum {
  V8_PROPERTY_ATTRIBUTE_NONE       = 0,       // Writeable, Enumerable,
                                              //   Configurable
  V8_PROPERTY_ATTRIBUTE_READONLY   = 1 << 0,  // Not writeable
  V8_PROPERTY_ATTRIBUTE_DONTENUM   = 1 << 1,  // Not enumerable
  V8_PROPERTY_ATTRIBUTE_DONTDELETE = 1 << 2   // Not configurable
} cef_v8_propertyattribute_t;

///
// Post data elements may represent either bytes or files.
///
typedef enum {
  PDE_TYPE_EMPTY  = 0,
  PDE_TYPE_BYTES,
  PDE_TYPE_FILE,
} cef_postdataelement_type_t;

///
// Resource type for a request.
///
typedef enum {
  ///
  // Top level page.
  ///
  RT_MAIN_FRAME = 0,

  ///
  // Frame or iframe.
  ///
  RT_SUB_FRAME,

  ///
  // CSS stylesheet.
  ///
  RT_STYLESHEET,

  ///
  // External script.
  ///
  RT_SCRIPT,

  ///
  // Image (jpg/gif/png/etc).
  ///
  RT_IMAGE,

  ///
  // Font.
  ///
  RT_FONT_RESOURCE,

  ///
  // Some other subresource. This is the default type if the actual type is
  // unknown.
  ///
  RT_SUB_RESOURCE,

  ///
  // Object (or embed) tag for a plugin, or a resource that a plugin requested.
  ///
  RT_OBJECT,

  ///
  // Media resource.
  ///
  RT_MEDIA,

  ///
  // Main resource of a dedicated worker.
  ///
  RT_WORKER,

  ///
  // Main resource of a shared worker.
  ///
  RT_SHARED_WORKER,

  ///
  // Explicitly requested prefetch.
  ///
  RT_PREFETCH,

  ///
  // Favicon.
  ///
  RT_FAVICON,

  ///
  // XMLHttpRequest.
  ///
  RT_XHR,
} cef_resource_type_t;

///
// Transition type for a request. Made up of one source value and 0 or more
// qualifiers.
///
typedef enum {
  ///
  // Source is a link click or the JavaScript window.open function. This is
  // also the default value for requests like sub-resource loads that are not
  // navigations.
  ///
  TT_LINK = 0,

  ///
  // Source is some other "explicit" navigation action such as creating a new
  // browser or using the LoadURL function. This is also the default value
  // for navigations where the actual type is unknown.
  ///
  TT_EXPLICIT = 1,

  ///
  // Source is a subframe navigation. This is any content that is automatically
  // loaded in a non-toplevel frame. For example, if a page consists of several
  // frames containing ads, those ad URLs will have this transition type.
  // The user may not even realize the content in these pages is a separate
  // frame, so may not care about the URL.
  ///
  TT_AUTO_SUBFRAME = 3,

  ///
  // Source is a subframe navigation explicitly requested by the user that will
  // generate new navigation entries in the back/forward list. These are
  // probably more important than frames that were automatically loaded in
  // the background because the user probably cares about the fact that this
  // link was loaded.
  ///
  TT_MANUAL_SUBFRAME = 4,

  ///
  // Source is a form submission by the user. NOTE: In some situations
  // submitting a form does not result in this transition type. This can happen
  // if the form uses a script to submit the contents.
  ///
  TT_FORM_SUBMIT = 7,

  ///
  // Source is a "reload" of the page via the Reload function or by re-visiting
  // the same URL. NOTE: This is distinct from the concept of whether a
  // particular load uses "reload semantics" (i.e. bypasses cached data).
  ///
  TT_RELOAD = 8,

  ///
  // General mask defining the bits used for the source values.
  ///
  TT_SOURCE_MASK = 0xFF,

  // Qualifiers.
  // Any of the core values above can be augmented by one or more qualifiers.
  // These qualifiers further define the transition.

  ///
  // Attempted to visit a URL but was blocked.
  ///
  TT_BLOCKED_FLAG = 0x00800000,

  ///
  // Used the Forward or Back function to navigate among browsing history.
  ///
  TT_FORWARD_BACK_FLAG = 0x01000000,

  ///
  // The beginning of a navigation chain.
  ///
  TT_CHAIN_START_FLAG = 0x10000000,

  ///
  // The last transition in a redirect chain.
  ///
  TT_CHAIN_END_FLAG = 0x20000000,

  ///
  // Redirects caused by JavaScript or a meta refresh tag on the page.
  ///
  TT_CLIENT_REDIRECT_FLAG = 0x40000000,

  ///
  // Redirects sent from the server by HTTP headers.
  ///
  TT_SERVER_REDIRECT_FLAG = 0x80000000,

  ///
  // Used to test whether a transition involves a redirect.
  ///
  TT_IS_REDIRECT_MASK = 0xC0000000,

  ///
  // General mask defining the bits used for the qualifiers.
  ///
  TT_QUALIFIER_MASK = 0xFFFFFF00,
} cef_transition_type_t;

///
// Flags used to customize the behavior of CefURLRequest.
///
typedef enum {
  ///
  // Default behavior.
  ///
  UR_FLAG_NONE                      = 0,

  ///
  // If set the cache will be skipped when handling the request.
  ///
  UR_FLAG_SKIP_CACHE                = 1 << 0,

  ///
  // If set user name, password, and cookies may be sent with the request, and
  // cookies may be saved from the response.
  ///
  UR_FLAG_ALLOW_CACHED_CREDENTIALS  = 1 << 1,

  ///
  // If set upload progress events will be generated when a request has a body.
  ///
  UR_FLAG_REPORT_UPLOAD_PROGRESS    = 1 << 3,

  ///
  // If set load timing info will be collected for the request.
  ///
  UR_FLAG_REPORT_LOAD_TIMING        = 1 << 4,

  ///
  // If set the headers sent and received for the request will be recorded.
  ///
  UR_FLAG_REPORT_RAW_HEADERS        = 1 << 5,

  ///
  // If set the CefURLRequestClient::OnDownloadData method will not be called.
  ///
  UR_FLAG_NO_DOWNLOAD_DATA          = 1 << 6,

  ///
  // If set 5XX redirect errors will be propagated to the observer instead of
  // automatically re-tried. This currently only applies for requests
  // originated in the browser process.
  ///
  UR_FLAG_NO_RETRY_ON_5XX           = 1 << 7,
} cef_urlrequest_flags_t;

///
// Flags that represent CefURLRequest status.
///
typedef enum {
  ///
  // Unknown status.
  ///
  UR_UNKNOWN = 0,

  ///
  // Request succeeded.
  ///
  UR_SUCCESS,

  ///
  // An IO request is pending, and the caller will be informed when it is
  // completed.
  ///
  UR_IO_PENDING,

  ///
  // Request was canceled programatically.
  ///
  UR_CANCELED,

  ///
  // Request failed for some reason.
  ///
  UR_FAILED,
} cef_urlrequest_status_t;

///
// Structure representing a rectangle.
///
typedef struct _cef_rect_t {
  int x;
  int y;
  int width;
  int height;
} cef_rect_t;

///
// Structure representing a size.
///
typedef struct _cef_size_t {
  int width;
  int height;
} cef_size_t;

///
// Existing process IDs.
///
typedef enum {
  ///
  // Browser process.
  ///
  PID_BROWSER,
  ///
  // Renderer process.
  ///
  PID_RENDERER,
} cef_process_id_t;

///
// Existing thread IDs.
///
typedef enum {
// BROWSER PROCESS THREADS -- Only available in the browser process.

  ///
  // The main thread in the browser. This will be the same as the main
  // application thread if CefInitialize() is called with a
  // CefSettings.multi_threaded_message_loop value of false.
  ///
  TID_UI,

  ///
  // Used to interact with the database.
  ///
  TID_DB,

  ///
  // Used to interact with the file system.
  ///
  TID_FILE,

  ///
  // Used for file system operations that block user interactions.
  // Responsiveness of this thread affects users.
  ///
  TID_FILE_USER_BLOCKING,

  ///
  // Used to launch and terminate browser processes.
  ///
  TID_PROCESS_LAUNCHER,

  ///
  // Used to handle slow HTTP cache operations.
  ///
  TID_CACHE,

  ///
  // Used to process IPC and network messages.
  ///
  TID_IO,

// RENDER PROCESS THREADS -- Only available in the render process.

  ///
  // The main thread in the renderer. Used for all WebKit and V8 interaction.
  ///
  TID_RENDERER,
} cef_thread_id_t;

///
// Supported value types.
///
typedef enum {
  VTYPE_INVALID = 0,
  VTYPE_NULL,
  VTYPE_BOOL,
  VTYPE_INT,
  VTYPE_DOUBLE,
  VTYPE_STRING,
  VTYPE_BINARY,
  VTYPE_DICTIONARY,
  VTYPE_LIST,
} cef_value_type_t;

///
// Supported JavaScript dialog types.
///
typedef enum {
  JSDIALOGTYPE_ALERT = 0,
  JSDIALOGTYPE_CONFIRM,
  JSDIALOGTYPE_PROMPT,
} cef_jsdialog_type_t;

///
// Screen information used when window rendering is disabled. This structure is
// passed as a parameter to CefRenderHandler::GetScreenInfo and should be filled
// in by the client.
///
typedef struct _cef_screen_info_t {
  ///
  // Device scale factor. Specifies the ratio between physical and logical
  // pixels.
  ///
  float device_scale_factor;

  ///
  // The screen depth in bits per pixel.
  ///
  int depth;

  ///
  // The bits per color component. This assumes that the colors are balanced
  // equally.
  ///
  int depth_per_component;

  ///
  // This can be true for black and white printers.
  ///
  int is_monochrome;

  ///
  // This is set from the rcMonitor member of MONITORINFOEX, to whit:
  //   "A RECT structure that specifies the display monitor rectangle,
  //   expressed in virtual-screen coordinates. Note that if the monitor
  //   is not the primary display monitor, some of the rectangle's
  //   coordinates may be negative values."
  //
  // The |rect| and |available_rect| properties are used to determine the
  // available surface for rendering popup views.
  ///
  cef_rect_t rect;

  ///
  // This is set from the rcWork member of MONITORINFOEX, to whit:
  //   "A RECT structure that specifies the work area rectangle of the
  //   display monitor that can be used by applications, expressed in
  //   virtual-screen coordinates. Windows uses this rectangle to
  //   maximize an application on the monitor. The rest of the area in
  //   rcMonitor contains system windows such as the task bar and side
  //   bars. Note that if the monitor is not the primary display monitor,
  //   some of the rectangle's coordinates may be negative values".
  //
  // The |rect| and |available_rect| properties are used to determine the
  // available surface for rendering popup views.
  ///
  cef_rect_t available_rect;
} cef_screen_info_t;

///
// Supported menu IDs. Non-English translations can be provided for the
// IDS_MENU_* strings in CefResourceBundleHandler::GetLocalizedString().
///
typedef enum {
  // Navigation.
  MENU_ID_BACK                = 100,
  MENU_ID_FORWARD             = 101,
  MENU_ID_RELOAD              = 102,
  MENU_ID_RELOAD_NOCACHE      = 103,
  MENU_ID_STOPLOAD            = 104,

  // Editing.
  MENU_ID_UNDO                = 110,
  MENU_ID_REDO                = 111,
  MENU_ID_CUT                 = 112,
  MENU_ID_COPY                = 113,
  MENU_ID_PASTE               = 114,
  MENU_ID_DELETE              = 115,
  MENU_ID_SELECT_ALL          = 116,

  // Miscellaneous.
  MENU_ID_FIND                = 130,
  MENU_ID_PRINT               = 131,
  MENU_ID_VIEW_SOURCE         = 132,

  // All user-defined menu IDs should come between MENU_ID_USER_FIRST and
  // MENU_ID_USER_LAST to avoid overlapping the Chromium and CEF ID ranges
  // defined in the tools/gritsettings/resource_ids file.
  MENU_ID_USER_FIRST          = 26500,
  MENU_ID_USER_LAST           = 28500,
} cef_menu_id_t;

///
// Mouse button types.
///
typedef enum {
  MBT_LEFT   = 0,
  MBT_MIDDLE,
  MBT_RIGHT,
} cef_mouse_button_type_t;

///
// Structure representing mouse event information.
///
typedef struct _cef_mouse_event_t {
  ///
  // X coordinate relative to the left side of the view.
  ///
  int x;

  ///
  // Y coordinate relative to the top side of the view.
  ///
  int y;

  ///
  // Bit flags describing any pressed modifier keys. See
  // cef_event_flags_t for values.
  ///
  uint32 modifiers;
} cef_mouse_event_t;

///
// Paint element types.
///
typedef enum {
  PET_VIEW  = 0,
  PET_POPUP,
} cef_paint_element_type_t;

///
// Supported event bit flags.
///
typedef enum {
  EVENTFLAG_NONE                = 0,
  EVENTFLAG_CAPS_LOCK_ON        = 1 << 0,
  EVENTFLAG_SHIFT_DOWN          = 1 << 1,
  EVENTFLAG_CONTROL_DOWN        = 1 << 2,
  EVENTFLAG_ALT_DOWN            = 1 << 3,
  EVENTFLAG_LEFT_MOUSE_BUTTON   = 1 << 4,
  EVENTFLAG_MIDDLE_MOUSE_BUTTON = 1 << 5,
  EVENTFLAG_RIGHT_MOUSE_BUTTON  = 1 << 6,
  // Mac OS-X command key.
  EVENTFLAG_COMMAND_DOWN        = 1 << 7,
  EVENTFLAG_NUM_LOCK_ON         = 1 << 8,
  EVENTFLAG_IS_KEY_PAD          = 1 << 9,
  EVENTFLAG_IS_LEFT             = 1 << 10,
  EVENTFLAG_IS_RIGHT            = 1 << 11,
} cef_event_flags_t;

///
// Supported menu item types.
///
typedef enum {
  MENUITEMTYPE_NONE,
  MENUITEMTYPE_COMMAND,
  MENUITEMTYPE_CHECK,
  MENUITEMTYPE_RADIO,
  MENUITEMTYPE_SEPARATOR,
  MENUITEMTYPE_SUBMENU,
} cef_menu_item_type_t;

///
// Supported context menu type flags.
///
typedef enum {
  ///
  // No node is selected.
  ///
  CM_TYPEFLAG_NONE        = 0,
  ///
  // The top page is selected.
  ///
  CM_TYPEFLAG_PAGE        = 1 << 0,
  ///
  // A subframe page is selected.
  ///
  CM_TYPEFLAG_FRAME       = 1 << 1,
  ///
  // A link is selected.
  ///
  CM_TYPEFLAG_LINK        = 1 << 2,
  ///
  // A media node is selected.
  ///
  CM_TYPEFLAG_MEDIA       = 1 << 3,
  ///
  // There is a textual or mixed selection that is selected.
  ///
  CM_TYPEFLAG_SELECTION   = 1 << 4,
  ///
  // An editable element is selected.
  ///
  CM_TYPEFLAG_EDITABLE    = 1 << 5,
} cef_context_menu_type_flags_t;

///
// Supported context menu media types.
///
typedef enum {
  ///
  // No special node is in context.
  ///
  CM_MEDIATYPE_NONE,
  ///
  // An image node is selected.
  ///
  CM_MEDIATYPE_IMAGE,
  ///
  // A video node is selected.
  ///
  CM_MEDIATYPE_VIDEO,
  ///
  // An audio node is selected.
  ///
  CM_MEDIATYPE_AUDIO,
  ///
  // A file node is selected.
  ///
  CM_MEDIATYPE_FILE,
  ///
  // A plugin node is selected.
  ///
  CM_MEDIATYPE_PLUGIN,
} cef_context_menu_media_type_t;

///
// Supported context menu media state bit flags.
///
typedef enum {
  CM_MEDIAFLAG_NONE                  = 0,
  CM_MEDIAFLAG_ERROR                 = 1 << 0,
  CM_MEDIAFLAG_PAUSED                = 1 << 1,
  CM_MEDIAFLAG_MUTED                 = 1 << 2,
  CM_MEDIAFLAG_LOOP                  = 1 << 3,
  CM_MEDIAFLAG_CAN_SAVE              = 1 << 4,
  CM_MEDIAFLAG_HAS_AUDIO             = 1 << 5,
  CM_MEDIAFLAG_HAS_VIDEO             = 1 << 6,
  CM_MEDIAFLAG_CONTROL_ROOT_ELEMENT  = 1 << 7,
  CM_MEDIAFLAG_CAN_PRINT             = 1 << 8,
  CM_MEDIAFLAG_CAN_ROTATE            = 1 << 9,
} cef_context_menu_media_state_flags_t;

///
// Supported context menu edit state bit flags.
///
typedef enum {
  CM_EDITFLAG_NONE            = 0,
  CM_EDITFLAG_CAN_UNDO        = 1 << 0,
  CM_EDITFLAG_CAN_REDO        = 1 << 1,
  CM_EDITFLAG_CAN_CUT         = 1 << 2,
  CM_EDITFLAG_CAN_COPY        = 1 << 3,
  CM_EDITFLAG_CAN_PASTE       = 1 << 4,
  CM_EDITFLAG_CAN_DELETE      = 1 << 5,
  CM_EDITFLAG_CAN_SELECT_ALL  = 1 << 6,
  CM_EDITFLAG_CAN_TRANSLATE   = 1 << 7,
} cef_context_menu_edit_state_flags_t;

///
// Key event types.
///
typedef enum {
  ///
  // Notification that a key transitioned from "up" to "down".
  ///
  KEYEVENT_RAWKEYDOWN = 0,

  ///
  // Notification that a key was pressed. This does not necessarily correspond
  // to a character depending on the key and language. Use KEYEVENT_CHAR for
  // character input.
  ///
  KEYEVENT_KEYDOWN,

  ///
  // Notification that a key was released.
  ///
  KEYEVENT_KEYUP,

  ///
  // Notification that a character was typed. Use this for text input. Key
  // down events may generate 0, 1, or more than one character event depending
  // on the key, locale, and operating system.
  ///
  KEYEVENT_CHAR
} cef_key_event_type_t;

///
// Structure representing keyboard event information.
///
typedef struct _cef_key_event_t {
  ///
  // The type of keyboard event.
  ///
  cef_key_event_type_t type;

  ///
  // Bit flags describing any pressed modifier keys. See
  // cef_event_flags_t for values.
  ///
  uint32 modifiers;

  ///
  // The Windows key code for the key event. This value is used by the DOM
  // specification. Sometimes it comes directly from the event (i.e. on
  // Windows) and sometimes it's determined using a mapping function. See
  // WebCore/platform/chromium/KeyboardCodes.h for the list of values.
  ///
  int windows_key_code;

  ///
  // The actual key code genenerated by the platform.
  ///
  int native_key_code;

  ///
  // Indicates whether the event is considered a "system key" event (see
  // http://msdn.microsoft.com/en-us/library/ms646286(VS.85).aspx for details).
  // This value will always be false on non-Windows platforms.
  ///
  int is_system_key;

  ///
  // The character generated by the keystroke.
  ///
  char16 character;

  ///
  // Same as |character| but unmodified by any concurrently-held modifiers
  // (except shift). This is useful for working out shortcut keys.
  ///
  char16 unmodified_character;

  ///
  // True if the focus is currently on an editable field on the page. This is
  // useful for determining if standard key events should be intercepted.
  ///
  int focus_on_editable_field;
} cef_key_event_t;

///
// Focus sources.
///
typedef enum {
  ///
  // The source is explicit navigation via the API (LoadURL(), etc).
  ///
  FOCUS_SOURCE_NAVIGATION = 0,
  ///
  // The source is a system-generated focus event.
  ///
  FOCUS_SOURCE_SYSTEM,
} cef_focus_source_t;

///
// Navigation types.
///
typedef enum {
  NAVIGATION_LINK_CLICKED = 0,
  NAVIGATION_FORM_SUBMITTED,
  NAVIGATION_BACK_FORWARD,
  NAVIGATION_RELOAD,
  NAVIGATION_FORM_RESUBMITTED,
  NAVIGATION_OTHER,
} cef_navigation_type_t;

///
// Supported XML encoding types. The parser supports ASCII, ISO-8859-1, and
// UTF16 (LE and BE) by default. All other types must be translated to UTF8
// before being passed to the parser. If a BOM is detected and the correct
// decoder is available then that decoder will be used automatically.
///
typedef enum {
  XML_ENCODING_NONE = 0,
  XML_ENCODING_UTF8,
  XML_ENCODING_UTF16LE,
  XML_ENCODING_UTF16BE,
  XML_ENCODING_ASCII,
} cef_xml_encoding_type_t;

///
// XML node types.
///
typedef enum {
  XML_NODE_UNSUPPORTED = 0,
  XML_NODE_PROCESSING_INSTRUCTION,
  XML_NODE_DOCUMENT_TYPE,
  XML_NODE_ELEMENT_START,
  XML_NODE_ELEMENT_END,
  XML_NODE_ATTRIBUTE,
  XML_NODE_TEXT,
  XML_NODE_CDATA,
  XML_NODE_ENTITY_REFERENCE,
  XML_NODE_WHITESPACE,
  XML_NODE_COMMENT,
} cef_xml_node_type_t;

///
// Popup window features.
///
typedef struct _cef_popup_features_t {
  int x;
  int xSet;
  int y;
  int ySet;
  int width;
  int widthSet;
  int height;
  int heightSet;

  int menuBarVisible;
  int statusBarVisible;
  int toolBarVisible;
  int locationBarVisible;
  int scrollbarsVisible;
  int resizable;

  int fullscreen;
  int dialog;
  cef_string_list_t additionalFeatures;
} cef_popup_features_t;

///
// DOM document types.
///
typedef enum {
  DOM_DOCUMENT_TYPE_UNKNOWN = 0,
  DOM_DOCUMENT_TYPE_HTML,
  DOM_DOCUMENT_TYPE_XHTML,
  DOM_DOCUMENT_TYPE_PLUGIN,
} cef_dom_document_type_t;

///
// DOM event category flags.
///
typedef enum {
  DOM_EVENT_CATEGORY_UNKNOWN = 0x0,
  DOM_EVENT_CATEGORY_UI = 0x1,
  DOM_EVENT_CATEGORY_MOUSE = 0x2,
  DOM_EVENT_CATEGORY_MUTATION = 0x4,
  DOM_EVENT_CATEGORY_KEYBOARD = 0x8,
  DOM_EVENT_CATEGORY_TEXT = 0x10,
  DOM_EVENT_CATEGORY_COMPOSITION = 0x20,
  DOM_EVENT_CATEGORY_DRAG = 0x40,
  DOM_EVENT_CATEGORY_CLIPBOARD = 0x80,
  DOM_EVENT_CATEGORY_MESSAGE = 0x100,
  DOM_EVENT_CATEGORY_WHEEL = 0x200,
  DOM_EVENT_CATEGORY_BEFORE_TEXT_INSERTED = 0x400,
  DOM_EVENT_CATEGORY_OVERFLOW = 0x800,
  DOM_EVENT_CATEGORY_PAGE_TRANSITION = 0x1000,
  DOM_EVENT_CATEGORY_POPSTATE = 0x2000,
  DOM_EVENT_CATEGORY_PROGRESS = 0x4000,
  DOM_EVENT_CATEGORY_XMLHTTPREQUEST_PROGRESS = 0x8000,
} cef_dom_event_category_t;

///
// DOM event processing phases.
///
typedef enum {
  DOM_EVENT_PHASE_UNKNOWN = 0,
  DOM_EVENT_PHASE_CAPTURING,
  DOM_EVENT_PHASE_AT_TARGET,
  DOM_EVENT_PHASE_BUBBLING,
} cef_dom_event_phase_t;

///
// DOM node types.
///
typedef enum {
  DOM_NODE_TYPE_UNSUPPORTED = 0,
  DOM_NODE_TYPE_ELEMENT,
  DOM_NODE_TYPE_ATTRIBUTE,
  DOM_NODE_TYPE_TEXT,
  DOM_NODE_TYPE_CDATA_SECTION,
  DOM_NODE_TYPE_PROCESSING_INSTRUCTIONS,
  DOM_NODE_TYPE_COMMENT,
  DOM_NODE_TYPE_DOCUMENT,
  DOM_NODE_TYPE_DOCUMENT_TYPE,
  DOM_NODE_TYPE_DOCUMENT_FRAGMENT,
} cef_dom_node_type_t;

///
// Supported file dialog modes.
///
typedef enum {
  ///
  // Requires that the file exists before allowing the user to pick it.
  ///
  FILE_DIALOG_OPEN = 0,

  ///
  // Like Open, but allows picking multiple files to open.
  ///
  FILE_DIALOG_OPEN_MULTIPLE,

  ///
  // Allows picking a nonexistent file, and prompts to overwrite if the file
  // already exists.
  ///
  FILE_DIALOG_SAVE,
} cef_file_dialog_mode_t;

///
// Geoposition error codes.
///
typedef enum {
  GEOPOSITON_ERROR_NONE = 0,
  GEOPOSITON_ERROR_PERMISSION_DENIED,
  GEOPOSITON_ERROR_POSITION_UNAVAILABLE,
  GEOPOSITON_ERROR_TIMEOUT,
} cef_geoposition_error_code_t;

///
// Structure representing geoposition information. The properties of this
// structure correspond to those of the JavaScript Position object although
// their types may differ.
///
typedef struct _cef_geoposition_t {
  ///
  // Latitude in decimal degrees north (WGS84 coordinate frame).
  ///
  double latitude;

  ///
  // Longitude in decimal degrees west (WGS84 coordinate frame).
  ///
  double longitude;

  ///
  // Altitude in meters (above WGS84 datum).
  ///
  double altitude;

  ///
  // Accuracy of horizontal position in meters.
  ///
  double accuracy;

  ///
  // Accuracy of altitude in meters.
  ///
  double altitude_accuracy;

  ///
  // Heading in decimal degrees clockwise from true north.
  ///
  double heading;

  ///
  // Horizontal component of device velocity in meters per second.
  ///
  double speed;

  ///
  // Time of position measurement in miliseconds since Epoch in UTC time. This
  // is taken from the host computer's system clock.
  ///
  cef_time_t timestamp;

  ///
  // Error code, see enum above.
  ///
  cef_geoposition_error_code_t error_code;

  ///
  // Human-readable error message.
  ///
  cef_string_t error_message;
} cef_geoposition_t;

///
// Print job color mode values.
///
typedef enum {
  COLOR_MODEL_UNKNOWN,
  COLOR_MODEL_GRAY,
  COLOR_MODEL_COLOR,
  COLOR_MODEL_CMYK,
  COLOR_MODEL_CMY,
  COLOR_MODEL_KCMY,
  COLOR_MODEL_CMY_K,  // CMY_K represents CMY+K.
  COLOR_MODEL_BLACK,
  COLOR_MODEL_GRAYSCALE,
  COLOR_MODEL_RGB,
  COLOR_MODEL_RGB16,
  COLOR_MODEL_RGBA,
  COLOR_MODEL_COLORMODE_COLOR,  // Used in samsung printer ppds.
  COLOR_MODEL_COLORMODE_MONOCHROME,  // Used in samsung printer ppds.
  COLOR_MODEL_HP_COLOR_COLOR,  // Used in HP color printer ppds.
  COLOR_MODEL_HP_COLOR_BLACK,  // Used in HP color printer ppds.
  COLOR_MODEL_PRINTOUTMODE_NORMAL,  // Used in foomatic ppds.
  COLOR_MODEL_PRINTOUTMODE_NORMAL_GRAY,  // Used in foomatic ppds.
  COLOR_MODEL_PROCESSCOLORMODEL_CMYK,  // Used in canon printer ppds.
  COLOR_MODEL_PROCESSCOLORMODEL_GREYSCALE,  // Used in canon printer ppds.
  COLOR_MODEL_PROCESSCOLORMODEL_RGB,  // Used in canon printer ppds
} cef_color_model_t;

///
// Print job duplex mode values.
///
typedef enum {
  DUPLEX_MODE_UNKNOWN = -1,
  DUPLEX_MODE_SIMPLEX,
  DUPLEX_MODE_LONG_EDGE,
  DUPLEX_MODE_SHORT_EDGE,
} cef_duplex_mode_t;

///
// Structure representing a print job page range.
///
typedef struct _cef_page_range_t {
  int from;
  int to;
} cef_page_range_t;

#ifdef __cplusplus
}
#endif

#endif  // CEF_INCLUDE_INTERNAL_CEF_TYPES_H_
