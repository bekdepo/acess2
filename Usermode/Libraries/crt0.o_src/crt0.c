/*
 * Acess2
 * - C Runtime 0 Common Code
 */

typedef	void	(*exithandler_t)(void);
typedef	void	(*constructor_t)(void);

exithandler_t	_crt0_exit_handler;
extern constructor_t	_crtbegin_ctors[];

int start(int argc, char *argv[], char **envp)
{
	 int	i;
	 int	rv;
	
	for( i = 0; _crtbegin_ctors[i]; i ++ )
		_crtbegin_ctors[i]();

	rv = main(argc, argv, envp);
	
	if( _crt0_exit_handler )
		_crt0_exit_handler();
	
	return rv;
}