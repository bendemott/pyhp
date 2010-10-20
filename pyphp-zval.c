/**
 * @author Caleb P Burns <cpburns2009@gmail.com>
 *
 *
 */

// stdio.h inplements malloc
#include <stdio.h>

// php_embed.h defines MAKE_STD_ZVAL, zval, zval types
#include <php_embed.h>

/**
 * Returns a PHP variable with the boolean value.
 *
 * @param bool value The boolean value to use.
 * @return zval* The PHP boolean variable.
 */
zval * pyphpZvalBool(bool value) {
	// Initialize zv.
	zval * zv;
	MAKE_STD_ZVAL(zv);
	// Set zv.return zv->value.str.val;
	zv->type = IS_BOOL;
	zv->value.lval = value ? 1 : 0;	
	return zv;
}

/**
 * Returns a PHP variable with the double floating-point value.
 *
 * @param double value The double floating-point value to use.
 * @return zval* The PHP double floating-point variable.
 */
zval * pyphpZvalDouble(double value) {
	// Initialize zv.
	zval * zv;
	MAKE_STD_ZVAL(zv);
	// Set zv.
	zv->type = IS_DOUBLE;
	zv->value.dval = value;
	return z;
}

/**
 * Returns a PHP variable with the long integer value.
 *
 * @param long value The long integer value to use.
 * @return zval* The PHP long integer variable.
 */
zval * pyphpZvalLong(long value) {
	// Initialize zv.
	zval * zv;
	MAKE_STD_ZVAL(zv);
	// Set zv.
	zv->type = IS_LONG;
	zv->value.lval = value;
	return zv;
}

/**
 * Returns a PHP variable with a null value.
 *
 * @return zval* The PHP null variable.
 */
zval * pyphpZvalNull() {
	// Initialize zv.
	zval * zv;
	MAKE_STD_ZVAL(zv);
	// Set zv.
	zv->type = IS_NULL;
	zv->value.lval = 0;
	return zv;
}

/**
 * Returns a PHP variable with a the character string.
 *
 * @param char* value The character string to use.
 * @return zval* The PHP character string variable.
 */
zval * pyphpZvalString(char * value) {
	// Initialize zv.
	zval * zv;
	MAKE_STD_ZVAL(zv);
	// Alloc and copy string.
	// NOTE: we could use the ZVAL_STRINGL macro specifying to duplicate which
	// calls estrndup() to duplicate the string.
	size_t length = strlen(value);
	char * string = (char *)malloc(sizeof(char)*(length+1));
	strncpy(string, value, length);
	string[length] = '\0';
	// Set zv.
	zv->type = IS_STRING;
	zv->value.str.val = string;
	zv->value.str.len = length;
	return zv;
}

/**
 * Returns the boolean value from the PHP variable.
 *
 * @param zval* The PHP variable to use.
 * @return bool The boolean value of the PHP variable.
 */
bool pyphpZvalToBool(zval * zv) {
	switch (zv->type) {
		case IS_LONG:
		case IS_BOOL:
			return zv->value.lval ? true : false;
		case IS_DOUBLE:
			return zv->value.dval ? true : false;
		case IS_STRING:
			switch (zv->value.str.len) {
				case 0:
					return false;
				case 1:
					return zv->value.str.val[0] != '0' ? true : false;
				default:
					return true;
			} 
			break;
		default:
			break;
	}
	return false;
}

/**
 * Returns the double floating-point value from the PHP variable.
 *
 * @todo Convert non-double values to double.
 *
 * @param zval* The PHP variable to use.
 * @return double The double floating-point value of the PHP variable.
 */
double pyphpZvalToDouble(zval * zv) {
	switch (zv->type) {
		case IS_LONG:
			return (double)zv->value.lval;
			break;
		case IS_DOUBLE:
			return zv->value.dval;
		case IS_BOOL:
			return zv->value.lval ? 1.0 : 0.0;
		case IS_STRING:
			return zend_string_to_double(zv->value.str.val, zv->value.str.len);
		default:
			break;
	}
	return 0.0;
}

/**
 * Returns the long integer value from the PHP variable.
 *
 * @param zval* The PHP variable to use.
 * @return long The long integer value of the PHP variable.
 */
long pyphpZvalToLong(zval * zv) {
	switch (zv->type) {
		case IS_LONG:
			return zv->value.lval;
		case IS_DOUBLE:
			return zend_dval_to_lval(zv->value.dval);
		case IS_BOOL:
			return zv->value.lval ? 1 : 0;
		case IS_STRING:
			return zend_atol(zv->value.str.val, zv->value.str.len);
		default:
			break;
	}
	return 0;
}

/**
 * Returns the pointer to a copy of the character string from the PHP variable.
 * The new string is allocated within this function. You are responsible for
 * freeing the memory associated with this character pointer.
 *
 * @todo Convert non-string values to strings.
 *
 * @param zval* The PHP variable to use.
 * @return char* The pointer a copy of the chatacter string of the PHP
 * variable.
 */
char * pyphpZvalToStringNew(zval * zv) {
	char * string;
	switch (zv->type) {
		case IS_LONG:
			//TODO
			// lval = Z_LVAL_P(zv);
			// Z_STRLEN_P(zv) = zend_spprintf(&Z_STRVAL_P(zv), 0, "%ld", lval);
			break;
		case IS_DOUBLE:
			//TODO
			// TSRMLS_FETCH();
			// dval = Z_DVAL_P(zv);
			// Z_STRLEN_P(zv) = zend_spprintf(&Z_STRVAL_P(zv), 0, "%.*G", (int) EG(precision), dval);
			break;
		case IS_BOOL: {
			string = (char *)malloc(sizeof(char)*2);
			string[0] = zv->value.lval ? '1' : '0';
			string[1] = '\0';
			return string;
		}
		case IS_STRING: {
			size_t length = zv->value.str.len;
			string = (char *)malloc(sizeof(char)*(length+1));
			strncpy(string, zv->value.str.val, length);
			string[length] = '\0';
			return string;
		}
		default:
			break;
	}
	string = (char *)malloc(sizeof(char));
	string[0] = '\0';
	return string;
}
