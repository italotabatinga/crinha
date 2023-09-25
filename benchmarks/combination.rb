def combination(n, k)
  a = k == 0
  b = k == n
  if (a || b)
    return 1
  else
    combination(n - 1, k - 1) + combination(n - 1, k)
  end
end

combination(15, 5)