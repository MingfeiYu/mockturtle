module top(x0, x1, x2, x3, x4, x5, x6, y0, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10, y11, y12, y13, y14, y15, y16, y17, y18, y19, y20, y21, y22, y23, y24, y25);
  input x0, x1, x2, x3, x4, x5, x6;
  output y0, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10, y11, y12, y13, y14, y15, y16, y17, y18, y19, y20, y21, y22, y23, y24, y25;
  wire n8, n9, n10, n11, n12, n13, n14, n15, n16, n17, n18, n19, n20, n21, n22, n23, n24, n25, n26, n27, n28, n29, n30, n31, n32, n33, n34, n35, n36, n37, n38, n39, n40, n41, n42, n43, n44, n45, n46, n47, n48, n49, n50, n51, n52, n53, n54, n55, n56, n57, n58, n59, n60, n61, n62, n63, n64, n65, n66, n67, n68, n69, n70, n71, n72, n73, n74, n75, n76, n77, n78, n79, n80, n81, n82, n83, n84, n85, n86, n87, n88, n89, n90, n91, n92, n93, n94, n95, n96, n97, n98, n99, n100, n101;
  assign n23 = x3 ^ x2;
  assign n13 = ~x2 & x3;
  assign n24 = n23 ^ n13;
  assign n25 = x4 ^ x1;
  assign n14 = ~x1 & x4;
  assign n26 = n25 ^ n14;
  assign n27 = n24 & n26;
  assign n8 = ~x3 & x4;
  assign n28 = n27 ^ n8;
  assign n17 = x1 & ~x3;
  assign n10 = x2 & x4;
  assign n18 = n10 ^ x2;
  assign n11 = n10 ^ x4;
  assign n12 = ~x0 & n11;
  assign n19 = n18 ^ n12;
  assign n20 = n17 & n19;
  assign n29 = n28 ^ n20;
  assign n30 = n29 ^ n8;
  assign n15 = n13 & ~n14;
  assign n16 = n15 ^ n13;
  assign n21 = n20 ^ n16;
  assign n22 = n12 & n21;
  assign n31 = n30 ^ n22;
  assign n9 = n8 ^ x4;
  assign n32 = n31 ^ n9;
  assign n33 = n32 ^ n27;
  assign n35 = n11 & n17;
  assign n34 = n31 ^ n27;
  assign n36 = n35 ^ n34;
  assign n39 = ~x1 & n13;
  assign n40 = n39 ^ n15;
  assign n37 = ~x2 & n8;
  assign n38 = n37 ^ n11;
  assign n41 = n40 ^ n38;
  assign n42 = n41 ^ n22;
  assign n49 = n8 ^ x3;
  assign n50 = n49 ^ n18;
  assign n51 = n50 ^ n24;
  assign n44 = n13 ^ x3;
  assign n43 = n38 ^ n9;
  assign n45 = n44 ^ n43;
  assign n46 = n45 ^ n18;
  assign n47 = n46 ^ n24;
  assign n48 = n47 ^ n41;
  assign n52 = n51 ^ n48;
  assign n53 = n52 ^ n32;
  assign n54 = x0 & n48;
  assign n55 = x5 & n9;
  assign n56 = n15 & n55;
  assign n57 = ~x0 & x1;
  assign n58 = n57 ^ x0;
  assign n59 = n58 ^ x1;
  assign n60 = n59 ^ x0;
  assign n61 = ~x6 & n60;
  assign n62 = n56 & ~n61;
  assign n63 = ~n54 & ~n62;
  assign n64 = x4 & ~x6;
  assign n65 = n13 & ~n64;
  assign n66 = n65 ^ n47;
  assign n67 = x1 & n66;
  assign n68 = n38 & ~n60;
  assign n69 = n68 ^ n51;
  assign n70 = x0 & n43;
  assign n71 = n70 ^ n34;
  assign n72 = x1 & n43;
  assign n73 = n72 ^ n20;
  assign n74 = n29 ^ n15;
  assign n75 = ~x3 & ~n58;
  assign n76 = ~n50 & n75;
  assign n77 = ~x1 & n19;
  assign n78 = ~x3 & n77;
  assign n79 = n78 ^ n50;
  assign n80 = ~x0 & n46;
  assign n81 = n80 ^ n46;
  assign n82 = n45 & ~n58;
  assign n83 = n45 & n59;
  assign n84 = n45 & n60;
  assign n85 = n45 & n57;
  assign n90 = n10 & ~n60;
  assign n86 = x4 & ~x5;
  assign n87 = ~x2 & ~n86;
  assign n88 = x0 & n87;
  assign n89 = ~n14 & n88;
  assign n91 = n90 ^ n89;
  assign n92 = x3 & n91;
  assign n93 = ~n64 & n87;
  assign n94 = n93 ^ n10;
  assign n95 = n60 & n94;
  assign n96 = n95 ^ n10;
  assign n97 = x3 & n96;
  assign n98 = n97 ^ n92;
  assign n99 = n37 & n59;
  assign n100 = n99 ^ n37;
  assign n101 = n100 ^ n30;
  assign y0 = n33;
  assign y1 = n36;
  assign y2 = n42;
  assign y3 = n53;
  assign y4 = ~n63;
  assign y5 = n67;
  assign y6 = n69;
  assign y7 = n71;
  assign y8 = n73;
  assign y9 = n21;
  assign y10 = n74;
  assign y11 = n76;
  assign y12 = n79;
  assign y13 = n81;
  assign y14 = n80;
  assign y15 = n82;
  assign y16 = n83;
  assign y17 = n84;
  assign y18 = n85;
  assign y19 = n46;
  assign y20 = n92;
  assign y21 = n98;
  assign y22 = n97;
  assign y23 = ~1'b0;
  assign y24 = n101;
  assign y25 = n99;
endmodule