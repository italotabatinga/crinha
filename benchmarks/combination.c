int combination(int n, int k) {
  int a = k == 0;
  int b = k == n;
  if (a || b) {
    return 1;
  } else {
    return combination(n - 1, k - 1) + combination(n - 1, k);
  }
}

int main(int argc, char const *argv[]) {
  combination(15, 5);
  return 0;
}
