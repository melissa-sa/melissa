import os, sys

class HiddenPrints:
        """Context manager used to suppress prints.
        Used to reduce visual bloat in Jupyter Notebook.
        """
        def __enter__(self):
            self._original_stdout = sys.stdout
            sys.stdout = open(os.devnull, 'w')
    
        def __exit__(self, exc_type, exc_val, exc_tb):
            sys.stdout.close()
            sys.stdout = self._original_stdout