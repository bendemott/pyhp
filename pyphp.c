/**
 * PyPHP is a python module written in c that embeds the PHP interpreter.
 *
 * @todo Convert functions using PyTuple_GetItem() to use PyArg_ParseTuple()
 *
 * @author Caleb P Burns <cpburns2009@gmail.com>
 * @author Ben DeMott <ben_demott@hotmail.com>
 * @date 2010-09-30
 * @version 0.4
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h> // for MAX_PATH

#include <Python.h>
#include <sapi/embed/php_embed.h>

#include "pyphp.h"
#include "pyphp-core.h"

// Python exception object.
PyObject * pyphp_exception = NULL;

// This is needed by php.
static PyMethodDef pyphpMethods[] = {
	{"shutdown", pyphp_shutdown, METH_VARARGS, "Shutdowns the PHP interpreter."},
	{"init", pyphp_init, METH_VARARGS, "Initializes the PHP interpreter."},
	{"displayErrors", pyphp_displayErrors, METH_VARARGS, "Sets whether PHP errors are displayed or not."},
	{"runInline", pyphp_runInline, METH_VARARGS, "Runs/evaluates an inline PHP script."},
	{"runScript", pyphp_runScript, METH_VARARGS, "Runs/executes a PHP script."},
	{"setVar", pyphp_setVar, METH_VARARGS, "Sets a global variable in PHP."},
	{"setSuperGlobalKey", pyphp_setSuperGlobalKey, METH_VARARGS, "Sets a super global variable in PHP."}
};

PyMODINIT_FUNC initpyphp(void) {
	PyObject * module = Py_InitModule("pyphp", pyphpMethods);
	PyObject * moduleDict = PyModule_GetDict(module);
	
	pyphp_exception = PyErr_NewException("pyphp.error", NULL, NULL);
	PyDict_SetItemString(moduleDict, "error", pyphp_exception);
	
	if (!pyphp_core_php_init(0, NULL)) {
		PyErr_SetString(pyphp_exception, "PHP failed to initialize!");
	}
}

/*******************************************************************************
 * Destroys/shuts-down the PHP interpreter.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* Always returns Py_None.
 ******************************************************************************/
static PyObject * pyphp_shutdown(PyObject * self, PyObject * args) {
	pyphp_core_php_shutdown();
	Py_RETURN_NONE;
}

/*******************************************************************************
 * Sets whether errors are displayed or not.
 *
 * Arguments:
 * - PyBool* displayErrors Whether errors are displayed or not.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* Always returns Py_None.
 ******************************************************************************/
static PyObject * pyphp_displayErrors(PyObject * self, PyObject * args) {
	const Py_ssize_t argCount = PyTuple_Size(args);
	if (argCount != 1) {
		PyErr_Format(PyExc_TypeError, "pyphp.displayErrors() takes 1 argument (%zd given)\n", argCount);
		return NULL;
	}
	
	PyObject * pyDisplayErrors = PyTuple_GetItem(args, 0);
	if (pyDisplayErrors == NULL || !PyBool_Check(pyDisplayErrors)) {
		PyErr_SetString(PyExc_TypeError, "1st argument is not True/False!");
		return NULL;
	}
	bool displayErrors = (PyObject_IsTrue(pyDisplayErrors) ? true : false);
	
	pyphp_core_php_displayErrors(displayErrors);
	
	Py_RETURN_NONE;
}

/*******************************************************************************
 * Initializes the PHP interpreter.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* Always returns Py_None.
 ******************************************************************************/
static PyObject * pyphp_init(PyObject * self, PyObject * args) {
	int init = pyphp_core_php_init(0, NULL);
	PyObject * pyReturn = init ? Py_True : Py_False;
	Py_INCREF(pyReturn);
	return pyReturn;
}

/*******************************************************************************
 * Runs/evaluates the PHP inline script.
 *
 * @todo Get filename of python script executing this inline php script.
 * @todo Display PHP errors.
 * @todo Raise Python error on PHP Fatal error.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* On success, Py_True; otherwise, Py_False.
 ******************************************************************************/
static PyObject * pyphp_runInline(PyObject * self, PyObject * args) {
	const Py_ssize_t argc = PySequence_Size(args);
	
	if (argc < 1) {
		printf("%s:%u No arguments passed!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	
	PyObject * pyInline = PyTuple_GetItem(args, 0);
	if (!PyString_Check(pyInline)) {
		printf("%s:%u 1st argument isn't a string!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	char * phpInline = PyString_AsString(pyInline);
	
	// Get filename of python script executing this inline php script.
	char filename[MAX_PATH + 1];
	// Before we can get the filename we must get the main module.
	PyObject * pyMain = PyImport_AddModule("__main__");
	if (pyMain != NULL) {
		// If the file attribute is present, get the object.
		if (PyObject_HasAttrString(pyMain, "__file__")) {
			PyObject * pyFile = PyObject_GetAttrString(pyMain, (const char *)"__file__"); // This seg faults if not cast to const char *
			if (pyFile != NULL) {
				if(PyString_Check(pyFile)) {
					char * name = PyString_AsString(pyFile);
					size_t len = strlen(name);
					memcpy(filename, name, len);
					filename[len] = '\0';
					name = NULL;
				}
				// If the file attribute isn't a string, that's very odd.
				else {
					memcpy(filename, "unknown\0", 8);
					printf("%s:%u Attribute __file__ in module __main__ is not a string\n", __FUNCTION__, __FILE__);
				}
				Py_DECREF(pyFile);
				pyFile = NULL;
			}
		}
		// Since the file attribute isn't present, we are running interactively or from the console.
		else {
			memcpy(filename, "console\0", 8);
		}
		Py_DECREF(pyMain);
		pyMain = NULL;
	} else {
		memcpy(filename, "unknown\0", 8);
		printf("%s:%u Invalid module __main__\n", __FUNCTION__, __LINE__);
	}
	
	// Execute inline script.
	int result = SUCCESS;
	zend_try {
		zend_eval_string(phpInline, NULL, filename TSRMLS_CC);
	} zend_catch {
		result = FAILURE;
	} zend_end_try();
	
	// Clean up variables.
	filename = NULL;
	phpInline = NULL;
	
	// Reset PHP.
	pyphp_core_php_reset();
	
	// Check for a python exception.
	PyObject * pyError = PyErr_Occurred();
	if (pyError != NULL) {
		return NULL;
	} else if (result == FAILURE) {
		PyErr_SetString(pyphp_exception, "An unknown PHP interpreter error occured");
	}
	
	PyObject * PyReturn = (result == SUCCESS ? Py_True : Py_False);
	Py_INCREF(PyReturn);
	return PyReturn;
}

/*******************************************************************************
 * Runs/executes the PHP script.
 *
 * @todo Display PHP errors.
 * @todo Raise Python error on PHP Fatal error.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* On success, Py_True; otherwise, Py_False.
 ******************************************************************************/
static PyObject * pyphp_runScript(PyObject * self, PyObject * args) {
	const Py_ssize_t argc = PyTuple_Size(args);
	
	if (argc < 1) {
		printf("%s:%u No arguments passed!", __FUNCTION__, __LINE__);
		return NULL;
	}
	
	PyObject * pyFile = PyTuple_GetItem(args, 0);
	char * filename;
	FILE * file;
	bool isFileOwned = false;
	if (PyString_Check(pyFile)) {
		filename = PyString_AsString(pyFile);
		file = fopen(filename, "rb");
		isFileOwned = true;
	} else if (PyFile_Check(pyFile)) {
		filename = PyString_AsString(PyFile_Name(pyFile));
		file = PyFile_AsFile(pyFile);
		PyFile_IncUseCount((PyFileObject *)pyFile);
	} else {
		printf("%s:%u 1st argument is neither a filename string nor a file pointer!", __FUNCTION__, __LINE__);
		return NULL;
	}
	
	// Create the zend file handle.
	zend_file_handle script;
	script.type = ZEND_HANDLE_FP;
	script.filename = filename;
	script.opened_path = NULL;
	script.free_filename = 0;
	script.handle.fp = file;
	
	// Execute the script.
	int result = SUCCESS;
	zend_try {
		zend_execute_scripts(ZEND_REQUIRE TSRMLS_CC, NULL, 1, &script);
	} zend_catch {
		result = FAILURE;
	} zend_end_try();
	
	// Clean up variables.
	// - NOTE: do not fclose() the file pointer because PHP will close the file
	//   pointer on its own.
	if (!isFileOwned) {
		PyFile_DecUseCount((PyFileObject *)pyFile);
	}
	
	file = NULL;
	filename = NULL;
	pyFile = NULL;
	
	// Reset PHP.
	pyphp_core_php_reset();
	
	// Check for a python exception.
	PyObject * pyError = PyErr_Occurred();
	if (pyError != NULL) {
		return NULL;
	} else if (result == FAILURE) {
		PyErr_SetString(pyphp_exception, "An unknown PHP interpreter error occured");
	}
	
	PyObject * PyReturn = (result == SUCCESS ? Py_True : Py_False);
	Py_INCREF(PyReturn);
	return PyReturn;
}

/*******************************************************************************
 * Sets a global PHP variable.
 *
 * @todo USE ZEND_SET_SYMBOL instead of zend_hash_update
 *
 * NOTE: Keys (variable) are limited to 255 character in length.
 *
 * Arguments:
 * - PyDict* dict A dict of key-value paris to set multiple variables in PHP.
 * - OR
 * - PyString* key The name of the variable to set in PHP.
 * - PyObject* value The value of the variable set in PHP.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* On success, Py_True; otherwise, Py_False.
 ******************************************************************************/
static PyObject * pyphp_setVar(PyObject * self, PyObject * args) {	
	const Py_ssize_t argc = PyTuple_Size(args);
	
	if (argc < 1) {
		printf("%s:%u No arguments passed!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	
	PyObject * pyItem = PyTuple_GetItem(args, 0);
	
	// Check to see if the first argument is a python dict.
	if (PyDict_Check(pyItem)) {
		// Iterate over python dict items, convert each item to a zval and set it as
		// as a global variable.
		Py_ssize_t pos = 0;
		PyObject * pyKey;
		PyObject * pyValue;
		zval * phpValue;
		char * key;
		char name[256];
		size_t len;
		while (PyDict_Next(pyItem, &pos, &pyKey, &pyValue)) {
			if (pyphp_core_convert_pyObjectToZval(pyValue, &phpValue)) {
				key = PyString_AsString(pyKey);
				// Ensure variable name (key) begins with a dollar sign ($).
				if (key[0] != '$') {
					PyErr_Format(PyExc_KeyError, "Invalid key in argument 1: %s - PHP variables must begin with a dollar sign ($)", key);
					return NULL;
				}
				// Strip the dollar sign.
				len = strlen(key)-1;
				if (len > 255) {
					PyErr_Format(PyExc_KeyError, "Invalid key length in argument 1: (%lu) %s  - PHP variable name must not exceed 255 characters", len, key);
					return NULL;
				}
				memcpy(name, &(key[1]), len);
				name[len] = '\0';
				
				ZEND_SET_GLOBAL_VAR(name, phpValue);
				zval_dtor(phpValue);
			}
		}
		key = NULL;
		phpValue = NULL;
		pyValue = NULL;
		pyKey = NULL;
		Py_RETURN_TRUE;
	} else if (argc >= 2 && PyString_Check(pyItem)) {
		PyObject * pyKey = pyItem;
		pyItem = PyTuple_GetItem(args, 1);
		zval * phpValue;
		char * key;
		char name[256];
		size_t len;
		if (pyphp_core_convert_pyObjectToZval(pyItem, &phpValue)) {
			key = PyString_AsString(pyKey);
			
			// Ensure variable name (key) begins with a dollar sign ($).
			if (key[0] != '$') {
				PyErr_Format(PyExc_KeyError, "Invalid name in argument 1: %s - PHP variables must begin with a dollar sign ($)", key);
				return NULL;
			}
			// Strip the dollar sign.
			len = strlen(key)-1;
			if (len > 255) {
				PyErr_Format(PyExc_KeyError, "Invalid name length in argument 1: (%lu) %s  - PHP variable name must not exceed 255 characters", len, key);
				return NULL;
			}
			memcpy(name, &(key[1]), len);
			name[len] = '\0';
			
			ZEND_SET_GLOBAL_VAR(name, phpValue);
			
			zend_print_zval_r(phpValue, 0);
		} else {
			printf("%s:%u Failed to convert python value to php value!\n", __FUNCTION__, __LINE__);
		}
		key = NULL;
		phpValue = NULL;
		pyKey = NULL;
		Py_RETURN_TRUE;
	} else {
		printf("%s:%u Invalid argument - argument 1:[key|dict] is a (%s), not a string|dict!\n", __FUNCTION__, __LINE__, pyItem->ob_type->tp_name);
	}
	
	pyItem = NULL;
	
	printf("%s:%u Arguments are invalid!\n", __FUNCTION__, __LINE__);
	Py_RETURN_FALSE;
}

/*******************************************************************************
 * Sets the PHP error handler callback function.
 *
 * Arguments:
 * - PyCallable* errorHandler() The PHP error handler callback function.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* Always returns Py_None.
 ******************************************************************************/
static PyObject * pyphp_setErrorHandler(PyObject * self, PyObject * args) {
	const Py_ssize_t argCount = PyTuple_Size(args);
	if (argCount < 1) {
		printf("%s:%u No arguments passed!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	
	PyObject * pyErrorHandler;
	
	if (PyArg_ParseTuple(args, "0:pyphp.setErrorHandler", &pyErrorHandler)) {
		if (!PyCallable_Check(pyErrorHandler)) {
			PyErr_SetString(PyExc_TypeError, "1st argument is not callable!");
			Py_RETURN_NONE;
		}
		pyphp_core_setErrorHandler(pyErrorHandler);
	}
	
	Py_RETURN_NONE;
}

/*******************************************************************************
 * Sets the PHP log handler callback function.
 *
 * Arguments:
 * - PyCallable* logHandler() The PHP log handler callback function.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* Always returns Py_None.
 ******************************************************************************/
static PyObject * pyphp_setLogHandler(PyObject * self, PyObject * args) {
	const Py_ssize_t argCount = PyTuple_Size(args);
	if (argCount < 1) {
		printf("%s:%u No arguments passed!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	
	PyObject * pyLogHandler;
	
	if (PyArg_ParseTuple(args, "0:pyphp.setLogHandler", &pyLogHandler)) {
		if (!PyCallable_Check(pyLogHandler)) {
			PyErr_SetString(PyExc_TypeError, "1st argument is not callable!");
			Py_RETURN_NONE;
		}
		pyphp_core_setLogHandler(pyLogHandler);
	}
	
	Py_RETURN_NONE;
}

/*******************************************************************************
 * Sets the PHP output handler callback function.
 *
 * Arguments:
 * - PyCallable* logHandler() The PHP output handler callback function.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* On success, Py_True; otherwise, Py_False;
 ******************************************************************************/
static PyObject * pyphp_setOutputHandler(PyObject * self, PyObject * args) {
	const Py_ssize_t argCount = PyTuple_Size(args);
	if (argCount < 1) {
		printf("%s:%u No arguments passed!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	
	PyObject * pyOutputHandler;
	
	if (PyArg_ParseTuple(args, "0:pyphp.setOutputHandler", &pyOutputHandler)) {
		if (!PyCallable_Check(pyOutputHandler)) {
			PyErr_SetString(PyExc_TypeError, "1st argument is not callable!");
			Py_RETURN_NONE;
		}
		pyphp_core_setOutputHandler(pyOutputHandler);
	}
	
	Py_RETURN_NONE;
}

/*******************************************************************************
 * Sets a super global PHP variable key.
 *
 * Arguments:
 * - PyString* superGlobal The PHP super global to modify.
 * - PyString* key The key of the super global to set.
 * - PyObject* value The value to set for the key.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* On success, Py_True; otherwise, Py_False.
 ******************************************************************************/
static PyObject * pyphp_setSuperGlobalKey(PyObject * self, PyObject * args) {
	const Py_ssize_t argc = PySequence_Size(args);
	if (argc < 3) {
		printf("%s:%u Invalid number of arguments - expected 3 arguments!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	
	// Get super global arg.
	PyObject * pySuperGlobal = PyTuple_GetItem(args, 0);
	if (!PyString_Check(pySuperGlobal)) {
		printf("%s:%u Invalid argument - argument 0:[superGlobal] is not a PyString!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	
	// Get key arg.
	PyObject * pyKey = PyTuple_GetItem(args, 1);
	if (!PyString_Check(pyKey)) {
		printf("%s:%u Invalid argument - argument 1:[key] is not a PyString!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	
	// Get value arg.
	PyObject * pyValue = PyTuple_GetItem(args, 2);
	
	// Convert python values to c and PHP values.
	char * superGlobal = PyString_AsString(pySuperGlobal);
	char * key = PyString_AsString(pyKey);
	zval * phpValue;
	
	pyphp_core_convert_pyObjectToZval(pyValue, &phpValue);
	
	// Find PHP super global.
	zval ** phpSuperGlobal;
	zend_hash_find(&EG(symbol_table), superGlobal, strlen(superGlobal), (void **)&phpSuperGlobal);
	//TODO: check zend_hash_find() return value.
	
	// Set super global key-value pair.
	ZEND_SET_SYMBOL(Z_ARRVAL(**phpSuperGlobal), key, phpValue);
	//TODO: check zend_hash_update() return value.

	// Clean up variables.
	*phpSuperGlobal = NULL;
	phpSuperGlobal = NULL;
	phpValue = NULL;
	key = NULL;
	superGlobal = NULL;
	pyValue = NULL;
	pyKey = NULL;
	pySuperGlobal = NULL;

	Py_RETURN_TRUE;
}
