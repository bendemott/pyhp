/**
 * pyphp-core.h provides the core functions used by the PyPHP module.
 *
 * @author Caleb P Burns <cpburns2009@gmail.com>
 * @author Ben DeMott <ben_demott@hotmail.com>
 * @date 2010-09-30
 * @version 0.4
 */
 
#include <stdio.h>
#include <stdarg.h>

#include <Python.h>
#include <sapi/embed/php_embed.h>

#include "pyphp-core.h"

extern PyObject * pyphp_exception;

/*******************************************************************************
 * Converts a Python object (PyObject) to a PHP value (zval).
 *
 * @param PyObject* pyObj The python object to convert.
 * @param zval** phpObj A reference to a zval where the converted zval will be
 * stored.
 * @return bool On success, true; otherwise, false.
 ******************************************************************************/
bool pyphp_core_convert_pyObjectToZval(PyObject * pyObj, zval ** phpObj) {
	// Make sure pyObj isn't NULL.
	if (pyObj == NULL) {
		printf("%s:%u PyObject is NULL!\n", __FUNCTION__, __LINE__);
		return false;
	}
	if (phpObj == NULL) {
		printf("%s:%u zval reference is NULL!\n", __FUNCTION__, __LINE__);
		return false;
	}
	
	// Check for Python null value.
	if (pyObj == Py_None) {
		MAKE_STD_ZVAL(*phpObj);
		zval * phpPtr = *phpObj;
		ZVAL_NULL(phpPtr);
		return true;
	}
	// Check for Python boolean value.
	else if (PyBool_Check(pyObj)) {
		MAKE_STD_ZVAL(*phpObj);
		zval * phpPtr = *phpObj;
		ZVAL_BOOL(phpPtr, PyInt_AsLong(pyObj));
		return true;
	}
	// Check for Python integer value.
	else if (PyInt_Check(pyObj) || PyLong_Check(pyObj)) {
		MAKE_STD_ZVAL(*phpObj);
		zval * phpPtr = *phpObj;
		ZVAL_LONG(phpPtr, PyInt_AsLong(pyObj));
		return true;
	}
	// Check for Python floating-point value.
	else if (PyFloat_Check(pyObj)) {
		MAKE_STD_ZVAL(*phpObj);
		zval * phpPtr = *phpObj;
		ZVAL_DOUBLE(phpPtr, PyFloat_AsDouble(pyObj));
		return true;
	}
	// Check for Python string.
	else if (PyString_Check(pyObj)) {
		MAKE_STD_ZVAL(*phpObj);
		zval * phpPtr = *phpObj;
		char * string = PyString_AsString(pyObj);
		ZVAL_STRING(phpPtr, string, 1);
		string = NULL;
		return true;
	}
	// Check for a Python unicode string.
	// TODO: allow multibyte strings for PHP.
	else if (PyUnicode_Check(pyObj)) {
		MAKE_STD_ZVAL(*phpObj);
		zval * phpPtr = *phpObj;
		PyObject * pyAscii = PyUnicode_AsASCIIString(pyObj);
		char * ascii = PyString_AsString(pyAscii);
		ZVAL_STRING(phpPtr, ascii, 1);
		Py_DECREF(pyAscii);
		ascii = NULL;
		pyAscii = NULL;
		return true;
	}
	// Check for Python dict.
	else if (PyDict_Check(pyObj)) {
		// Initialize the PHP object to an array.
		MAKE_STD_ZVAL(*phpObj);
		zval * phpPtr = *phpObj;
		array_init(phpPtr);
		// Iterate over the python dict, convert the python keys and values into PHP
		// keys and values, and append the key-value pairs to the PHP object.
		PyObject * pyKey;
		PyObject * pyValue;
		Py_ssize_t pos = 0;
		zval * phpValue;
		char * key;
		while (PyDict_Next(pyObj, &pos, &pyKey, &pyValue)) {
			// Convert the PyObject value into the PHP value.
			if (pyphp_core_convert_pyObjectToZval(pyValue, &phpValue)) {
				// Convert the PyObject key into the PHP string.
				// Append the PHP key-value pair to the PHP object.
				key = PyString_AsString(pyKey);
				if (zend_hash_update(Z_ARRVAL_P(phpPtr), key, strlen(key), (void *)&phpValue, sizeof(phpValue), NULL) == FAILURE) {
					printf("%s:%u Failed to insert zval into hash\n", __FUNCTION__, __LINE__);
					zval_dtor(phpValue);
					phpValue = NULL;
				}
			} else {
				printf("%s:%u Failed to convert python value to php value\n", __FUNCTION__, __LINE__);
				if (phpValue != NULL) {
					zval_dtor(phpValue);
					phpValue = NULL;
				}
				return false;
			}
		}
		key = NULL;
		phpValue = NULL;
		pyValue = NULL;
		pyKey = NULL;
		
		return true;
	}
	// Check for Python list/tuple.
	else if (PySequence_Check(pyObj)) {
		// Initialize the PHP object to an array.
		MAKE_STD_ZVAL(*phpObj);
		zval * phpPtr = *phpObj;
		array_init(phpPtr);
		// Iterate over the python sequence items, convert the python items into
		// PHP values, and append the items to the PHP object.
		const Py_ssize_t seqSize = PySequence_Size(pyObj);
		PyObject * pyItem;
		zval * phpItem;
		Py_ssize_t i;
		for (i = 0; i < seqSize; i++) {
			// Get the python sequence item at index.
			pyItem = PySequence_GetItem(pyObj, i);
			if (pyItem != NULL) {
				// Convert the PyObject item into the PHP item.
				phpItem = NULL;
				if (pyphp_core_convert_pyObjectToZval(pyItem, &phpItem)) {
					// Append the PHP item to the PHP object.
					//add_next_index_zval(phpPtr, phpItem);
					if (zend_hash_next_index_insert(Z_ARRVAL_P(phpPtr), &phpItem, sizeof(phpItem), NULL) == FAILURE) {
						printf("%s:%u Failed to insert zval into hash\n", __FUNCTION__, __LINE__);
						zval_dtor(phpItem);
						return false;
					}
				} else {
					printf("%s:%u Failed to convert python value to php value\n", __FUNCTION__, __LINE__);
					if (phpItem != NULL) {
						zval_dtor(phpItem);
						phpItem = NULL;
					}
					return false;
				}
				// Delete the reference to the python item.
				Py_DECREF(pyItem);
			}
		}
		phpItem = NULL;
		pyItem = NULL;
		
		return true;
	}
	
	printf("%s:%u The PyObject's type is not supported by this function!", __FUNCTION__, __LINE__);
	return false;
}

/*******************************************************************************
 * The PHP error handler.
 *
 * @param int type The error type.
 * @param char* file The PHP file the error occured in.
 * @param uint line The line the error occured on.
 * @param char* format The format?
 * @param va-list args The Variadic arguments for the format.
 ******************************************************************************/
void pyphp_core_php_errorHandler(int type, const char * file, const unsigned int line, const char * format, va_list args) {
	char * error;
	bool isFatal = false;
	switch (type) {
		case E_ERROR:
		case E_CORE_ERROR:
		case E_COMPILE_ERROR:
		case E_USER_ERROR:
			isFatal = true;
			error = "Fatal error";
			break;
		case E_RECOVERABLE_ERROR:
			error = "Catchable fatal error";
			break;
		case E_WARNING:
		case E_CORE_WARNING:
		case E_COMPILE_WARNING:
		case E_USER_WARNING:
			error = "Warning";
			break;
		case E_PARSE:
			error = "Parse error";
			break;
		case E_NOTICE:
		case E_USER_NOTICE:
			error = "Notice";
			break;
		case E_STRICT:
			error = "Strict Standards";
			break;
		case E_DEPRECATED:
		case E_USER_DEPRECATED:
			error = "Deprecated";
			break;
		default:
			error = "Unknown error";
			break;
	}
	
	// If the error is fatal, raise a python exception.
	if (isFatal) {
		// Copy args so that the args don't become corrupted when passed to the
		// internal PHP error handler.
		va_list vars;
		va_copy(vars, args);
		char * message;
		vspprintf(&message, PG(log_errors_max_len), format, vars);
		
		PyErr_Format(pyphp_exception, "PHP %s: %s in %s on line %u", error, message, file, line);
		
		// Clean up.
		efree(message);
		message = NULL;
	}
	
	// Call the internal PHP error handler.
	pyphp_core.phpInternalErrorHandler(type, file, line, format, args);
}

/*******************************************************************************
 * The PHP log handler.
 *
 * @param char* messsage The log message.
 ******************************************************************************/
void pyphp_core_php_logHandler(char * message) {
	// Call the PHP log handler python callback function if it's set.
	if (pyphp_core.pyLogHandler) {
		PyObject * pyArgs = Py_BuildValue("{s:s}:pyphp.pyphp_core_php_logHandler", "message", message);
		PyObject * pyResult = PyEval_CallObject(pyphp_core.pyOutputHandler, pyArgs);
		Py_XDECREF(pyResult);
		Py_DECREF(pyArgs);
		pyResult = NULL;
		pyArgs = NULL;
		return;
	}
	
	// Since no log handler was specified, send log message to the log stream
	// (usually stdout or stdlog).
	//fprintf(pyphp_core.logStream, "%s\n", message);
	fflush(pyphp_core.logStream);
}

/*******************************************************************************
 * The PHP output handler.
 *
 * @param char* messsage The output message.
 * @return int The number of characters written.
 ******************************************************************************/
int pyphp_core_php_outputHandler(const char * message, unsigned int length TSRMLS_DC) {
	// Call the PHP output handler python callback function if it's set.
	if (pyphp_core.pyOutputHandler) {
		PyObject * pyArgs = Py_BuildValue("{s:s}:pyphp.pyphp_core_php_outputHandler", "message", message);
		PyObject * pyResult = PyEval_CallObject(pyphp_core.pyOutputHandler, pyArgs);
		Py_XDECREF(pyResult);
		Py_DECREF(pyArgs);
		pyResult = NULL;
		pyArgs = NULL;
		return length;
	}
	
	// Since no output handler was specified, send output message to the output
	// stream (usually stdout).
	fwrite(message, sizeof(char), length, pyphp_core.outputStream);
	fflush(pyphp_core.outputStream);
	return length;
}

/*******************************************************************************
 * The PHP output flush handler.
 ******************************************************************************/
void pyphp_core_php_outputFlushHandler(void * server_context) {
	fflush(pyphp_core.outputStream);
}

/*******************************************************************************
 * The PHP startup handler.
 *
 * @hack Stores and overrides the internal PHP error handler.
 *
 * @param sapi_module_struct* sapi_module The SAPI module to startup.
 * @return int On success, SUCCESS (0); otherwise, FAILURE (1).
 ******************************************************************************/
int pyphp_core_php_startup(sapi_module_struct * sapi_module) {
	if (php_module_startup(sapi_module, NULL, 0) == FAILURE) {
		return FAILURE;
	}
	//HACK: Grab the function pointer to the internal PHP (zend) error handler.
	pyphp_core.phpInternalErrorHandler = zend_error_cb;
	//HACK: Override the internal PHP error handler.
	zend_error_cb = pyphp_core_php_errorHandler;
	
	return SUCCESS;
}
