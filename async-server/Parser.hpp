


/*
 * This function looks at method used and calls the appropriate handler
 * function. Since we only implement GET and POST methods, it calls
 * handle_unimplemented_method() in case both these don't match. This sends an
 * error to the client.
 * */
