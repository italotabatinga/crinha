def fibTco(n, a, b):
  if (n == 0):
    return a
  else:
    if (n == 1):
      return b
    else:
      return fibTco(n - 1, b, a+ b)

fibTco(46, 0, 1)