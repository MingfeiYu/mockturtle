module top(x0, x1, x2, y0, y1);
  input x0, x1, x2;
  output y0, y1;
  wire n0, n1, n2, n3, n4, n5, n6, n7, n8, n9;
  assign n0 = x0 ^ x1;
  assign n1 = n0 ^ x2;
  assign y0 = n1;
  assign n2 = ~x2;
  assign n3 = n0 & n2;
  assign n4 = ~n3;
  assign n5 = ~x0;
  assign n6 = ~x1;
  assign n7 = n5 & n6;
  assign n8 = ~n7;
  assign n9 = n4 & n8;
  assign y1 = ~n9;
endmodule
