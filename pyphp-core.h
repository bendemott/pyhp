/**
 * pyphp-core.h provides the core functions used by the PyPHP module.
 *
 * @author Caleb P Burns <cpburns2009@gmail.com>
 * @author Ben DeMott <ben_demott@hotmail.com>
 * @date 2010-09-30
 * @version 0.4
 */

#include <stdio.h>
#include <stdbool.h>

#include <Python.h>
#include <sapi/embed/php_embed.h>

// Needed by PHP embed.
#ifdef ZTS
void **** ptsrm_ls;
#endif

struct pyphp_core_t {
	bool isInit;
	// Output streams.
	FILE * logStream;
	FILE * errorStream;
	FILE * outputStream;
	// Python callback functions.
	PyObject * pyErrorHandler;
	PyObject * pyLogHandler;
	PyObject * pyOutputHandler;
	// Internal PHP (zend) error function.
	void (* phpInternalErrorHandler)(int type, const char * file, const uint line, const char * format, va_list args) ZEND_ATTRIBUTE_PTR_FORMAT(printf, 4, 0);
} pyphp_core;

/*******************************************************************************
 * Converts a Python value (PyObject) to a PHP value (zval).
 *
 * @param PyObject* pyObj The python object to convert.
 * @param zval** phpObj A reference to a zval where the converted zval will be
 * stored.
 * @return bool On success, true; otherwise, false.
 ******************************************************************************/
bool pyphp_core_convert_pyObjectToZval(PyObject * pyObj, zval ** phpObj);

/*******************************************************************************
 * The PHP error handler.
 *
 * @param int type The error type.
 * @param char* file The PHP file the error occured in.
 * @param uint line The line the error occured on.
 * @param char* format The format?
 * @param va-list args The Variadic arguments for the format.
 ******************************************************************************/
void pyphp_core_php_errorHandler(int type, const char * file, const unsigned int line, const char * format, va_list args);

/*******************************************************************************
 * The PHP log handler.
 *
 * @param char* messsage The log message.
 ******************************************************************************/
void pyphp_core_php_logHandler(char * message);

/*******************************************************************************
 * The PHP output handler.
 *
 * @param char* messsage The output message.
 * @param int The number of characters written.
 ******************************************************************************/
int pyphp_core_php_outputHandler(const char * message, unsigned int length TSRMLS_DC);

/*******************************************************************************
 * The PHP output flush handler.
 ******************************************************************************/
void pyphp_core_php_outputFlushHandler(void * server_context);

/*******************************************************************************
 * The PHP startup handler.
 *
 * @hack Stores and overrides the internal PHP error handler.
 *
 * @param sapi_module_struct* sapi_module The SAPI module to startup.
 * @return int On success, SUCCESS (0); otherwise, FAILURE (1).
 ******************************************************************************/
int pyphp_core_php_startup(sapi_module_struct * sapi_module);

/*******************************************************************************
 * Sets whether PHP prints its errors to stdout or not.
 *
 * @param bool displayErrors Whether PHP prints errors or not.
 ******************************************************************************/
static inline void pyphp_core_php_displayErrors(bool displayErrors) {
	zend_alter_ini_entry("display_errors", sizeof("display_errors"), (displayErrors ? "1" : "0"), 1, PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME);
}

/*******************************************************************************
 * Destroys/shuts-down the PHP interpreter.
 ******************************************************************************/
static inline void pyphp_core_php_shutdown() {
	if (!pyphp_core.isInit) {
		return;
	}
	pyphp_core.isInit = false;
	php_embed_shutdown();
}

/*******************************************************************************
 * Sets-up the PHP INI settings.
 ******************************************************************************/
static inline void pyphp_core_php_setup_ini(void) {
	char allErrors[12];
	snprintf(allErrors, sizeof(allErrors)-1, "%u", E_ALL);
	
	zend_alter_ini_entry("error_reporting", sizeof("error_reporting"), allErrors, strlen(allErrors), PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME);
	zend_alter_ini_entry("log_errors", sizeof("log_errors"), "1", 1, PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME);
	zend_alter_ini_entry("display_errors", sizeof("display_errors"), "1", 1, PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME);
	zend_alter_ini_entry("display_startup_errors", sizeof("display_startup_errors"), "1", 1, PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME);
}

/*******************************************************************************
 * Initializes the PHP interpreter.
 *
 * @param int argc The number of arguments sent.
 * @param char** argv An array of character string arguments to send to PHP.
 * @return bool On success, true; otherwise, false.
 ******************************************************************************/
static inline bool pyphp_core_php_init(int argc, char ** argv) {
	if (pyphp_core.isInit) {
		return true;
	}
	pyphp_core.isInit = true;
	pyphp_core.logStream = stdout;
	pyphp_core.errorStream = stdout;
	pyphp_core.outputStream = stdout;
	
	// Override PHP embed log handler.
	php_embed_module.log_message = pyphp_core_php_logHandler;
	
	// Override PHP embed output handler.
	php_embed_module.ub_write = pyphp_core_php_outputHandler;
	
	// Override PHP embed output flush handler.
	php_embed_module.flush = pyphp_core_php_outputFlushHandler;
	
	// Override PHP embed startup handler.
	php_embed_module.startup = pyphp_core_php_startup;
	
	// Initialize PHP.
	if (php_embed_init(argc, argv PTSRMLS_CC) == FAILURE) {
		printf("%s:%u Failed to initialize PHP!\n", __FUNCTION__, __LINE__);
		return false;
	}
	//EG(bailout_set) = 0;
	
	// Setup INI.
	pyphp_core_php_setup_ini();
	
	return true;
}

/*******************************************************************************
 * Resets the PHP interpreter.
 *
 * @return bool On success, true; otherwise, false.
 ******************************************************************************/
static inline bool pyphp_core_php_reset(void) {
	php_request_shutdown(NULL);
	if (php_request_startup(TSRMLS_C) == FAILURE) {
		printf("%s:%u Failed to re-startup the PHP!\n", __FUNCTION__, __LINE__);
		return false;
	}
	
	// Setup INI.
	pyphp_core_php_setup_ini();
	
	return true;
}
 
/*******************************************************************************
 * Sets the PHP error handler callback function.
 *
 * @param PyObject* pyErrorHandler The Python error handler callback function.
 ******************************************************************************/
static inline void pyphp_core_setErrorHandler(PyObject * pyErrorHandler) {
	Py_XDECREF(pyphp_core.pyErrorHandler);
	Py_XINCREF(pyErrorHandler);
	pyphp_core.pyErrorHandler = pyErrorHandler;
}

/*******************************************************************************
 * Sets the PHP log handler callback function.
 *
 * @param PyObject* pyLogHandler The Python log handler callback function.
 ******************************************************************************/
static inline void pyphp_core_setLogHandler(PyObject * pyLogHandler) {
	Py_XDECREF(pyphp_core.pyLogHandler);
	Py_XINCREF(pyLogHandler);
	pyphp_core.pyLogHandler = pyLogHandler;
}

/*******************************************************************************
 * Sets the PHP output handler callback function.
 *
 * @param PyObject* pyOutputHandler The Python output handler callback function.
 ******************************************************************************/
static inline void pyphp_core_setOutputHandler(PyObject * pyOutputHandler) {
	Py_XDECREF(pyphp_core.pyOutputHandler);
	Py_XINCREF(pyOutputHandler);
	pyphp_core.pyOutputHandler = pyOutputHandler;
}
