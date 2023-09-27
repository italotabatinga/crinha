int fibTco(int n, int a, int b) {
  if (n == 0) {
    return a;
  } else {
    if (n == 1) {
      return b;
    } else {
      return fibTco(n - 1, b, a + b);
    }
  }
}

int main(int argc, char const *argv[]) {
  fibTco(46, 0, 1);
  return 0;
}
