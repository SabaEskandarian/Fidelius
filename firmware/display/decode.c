#include <Python.h>
#include <string.h>


static PyObject *
decode_system(PyObject *self, PyObject *args)

{

    const char* command;
    Py_ssize_t count;
    unsigned char green = 0;


    if (!PyArg_ParseTuple(args, "s#b", &command, &count, &green))
        return NULL;

    //int x = count;
    //printf("a");
    printf("count: %zd\n", count);

    char buff[count*32];
    for (int i = 0; i < count; i++)
    {
    	unsigned char byte = command[i];
    	for (int x = 0; x < 8; x++) 
    	{
    		if (byte & 0x01) 
    		{
    			buff[i*32 + x*4] = (char)0x48;
    			buff[i*32 + x*4 + 1] = (char)0x8b;
    			buff[i*32 + x*4 + 2] = (char)0x49;
    			buff[i*32 + x*4 + 3] = green ? (char)255 : (char)0;
    		} else {
    			buff[i*32 + x*4] = green ? (char)255 : (char)0;
    			buff[i*32 + x*4 + 1] = green ? (char)255 : (char)0;
    			buff[i*32 + x*4 + 2] = green ? (char)255 : (char)0;
    			buff[i*32 + x*4 + 3] = (char)255;
    		}
    		byte = byte >> 1;
    	}
    }
    return Py_BuildValue("s#", buff, count*32);
}

static PyMethodDef DecodeMethods[] = {
        {"system",  decode_system, METH_VARARGS,
     "Execute a shell command."},
        {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initdecode(void)
{
    (void) Py_InitModule("decode", DecodeMethods);
}

int
main(int argc, char *argv[])
{
    /* Pass argv[0] to the Python interpreter */
    Py_SetProgramName(argv[0]);

    /* Initialize the Python interpreter.  Required. */
    Py_Initialize();

    /* Add a static module */
    initdecode();
}
