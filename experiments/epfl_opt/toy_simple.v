module top(x0, x1, x2, x3, x4, x5, y0);
  input x0, x1, x2, x3, x4, x5;
  output y0;
  wire n0, n1, n2, n3, n4;
  assign n0 = x0 ^ x1;
  assign n1 = x2 ^ x3;
  assign n2 = n0 & n1;
  assign n3 = x4 & x5;
  assign n4 = n2 & n3;
  assign y0 = n4;
endmodule
