def fib_tco(n, a, b)
  if (n == 0):
    a
  else
    if (n == 1):
      b
    else
      fib_tco(n - 1, b, a + b)
    end
  end
end

fib_tco(46, 0, 1)