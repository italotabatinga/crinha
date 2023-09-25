def combination(n, k):
  a = k == 0
  b = k == n
  if a or b:
    return 1
  else:
    combination(n - 1, k - 1) + combination(n - 1, k)

combination(15, 5)