/**
 * PyPHP is a python module written in c that embeds the PHP interpreter.
 *
 * @author Caleb P Burns <cpburns2009@gmail.com>
 * @author Ben DeMott <ben_demott@hotmail.com>
 * @date 2010-09-30
 * @version 0.4
 */
 
// Python.h implements PyObject
#include <Python.h>
#include <sapi/embed/php_embed.h>

/**
 * Destroys/shuts-down the PHP interpreter.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* Always returns Py_None.
 */
static PyObject * pyphp_shutdown(PyObject * self, PyObject * args);

/**
 * Sets whether errors are displayed or not.
 *
 * Arguments:
 * - PyBool* displayErrors Whether errors are displayed or not.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* Always returns Py_None.
 */
static PyObject * pyphp_displayErrors(PyObject * self, PyObject * args);

/**
 * Initializes the PHP interpreter.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* Always returns Py_None.
 */
static PyObject * pyphp_init(PyObject * self, PyObject * args);

/**
 * Runs/evaluates the PHP inline script.
 *
 * @todo Get filename of python script executing this inline php script.
 * @todo Display PHP errors.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* On success, Py_True; otherwise, Py_False.
 */
static PyObject * pyphp_runInline(PyObject * self, PyObject * args);

/**
 * Runs/executes the PHP script.
 *
 * @todo Display PHP errors.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* On success, Py_True; otherwise, Py_False.
 */
static PyObject * pyphp_runScript(PyObject * self, PyObject * args);

/**
 * Sets the PHP error handler callback function.
 *
 * Arguments:
 * - PyCallable* errorHandler() The PHP error handler callback function.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* Always returns Py_None.
 */
static PyObject * pyphp_setErrorHandler(PyObject * self, PyObject * args);

/**
 * Sets the PHP log handler callback function.
 *
 * Arguments:
 * - PyCallable* logHandler() The PHP log handler callback function.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* Always returns Py_None.
 */
static PyObject * pyphp_setLogHandler(PyObject * self, PyObject * args);

/**
 * Sets the PHP output handler callback function.
 *
 * Arguments:
 * - PyCallable* logHandler() The PHP output handler callback function.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* On success, Py_True; otherwise, Py_False;
 */
static PyObject * pyphp_setOutputHandler(PyObject * self, PyObject * args);

/**
 * Sets a global PHP variable.
 *
 * @param PyObject* self Myself.
 * @param PyObject* args The function arguments.
 * @return PyObject* On success, Py_True; otherwise, Py_False.
 */
static PyObject * pyphp_setVar(PyObject * self, PyObject * args);

/**
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
 */
static PyObject * pyphp_setSuperGlobalKey(PyObject * self, PyObject * args);
