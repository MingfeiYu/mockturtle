module top( x0 , x1 , x2 , x3 , x4 , x5 , x6 , x7 , y0 , y1 );
  input x0 , x1 , x2 , x3 , x4 , x5 , x6 , x7 ;
  output y0 , y1 ;
  wire n9 , n10 , n11 , n12 , n13 , n14 , n15 , n16 , n17 , n18 , n19 , n20 , n21 , n22 , n23 , n24 , n25 , n26 , n27 , n28 , n29 , n30 , n31 , n32 , n33 , n34 , n35 , n36 , n37 , n38 , n39 , n40 , n41 , n42 , n43 , n44 , n45 , n46 , n47 , n48 , n49 , n50 , n51 , n52 , n53 , n54 , n55 , n56 , n57 , n58 , n59 , n60 , n61 , n62 , n63 , n64 , n65 , n66 , n67 , n68 , n69 , n70 , n71 , n72 , n73 , n74 , n75 , n76 , n77 , n78 , n79 , n80 , n81 , n82 , n83 , n84 , n85 , n86 , n87 , n88 , n89 , n90 , n91 , n92 , n93 , n94 , n95 , n96 , n97 , n98 , n99 , n100 , n101 , n102 , n103 , n104 , n105 , n106 , n107 , n108 , n109 , n110 , n111 , n112 ;
  assign n9 = x0 & x1 ;
  assign n10 = x6 & x7 ;
  assign n11 = x4 & x5 ;
  assign n12 = x2 & x3 ;
  assign n13 = ~n11 & ~n12 ;
  assign n14 = n11 & n12 ;
  assign n15 = ~n13 & ~n14 ;
  assign n16 = n10 & ~n15 ;
  assign n17 = ~n10 & n15 ;
  assign n18 = ~n16 & ~n17 ;
  assign n19 = n9 & ~n18 ;
  assign n20 = ~n10 & ~n14 ;
  assign n21 = ~n13 & ~n20 ;
  assign n22 = n19 & n21 ;
  assign n23 = ~n19 & ~n21 ;
  assign n24 = ~n22 & ~n23 ;
  assign n25 = ~x0 & x1 ;
  assign n26 = ~x6 & x7 ;
  assign n27 = ~x4 & x5 ;
  assign n28 = ~x2 & x3 ;
  assign n29 = ~n27 & ~n28 ;
  assign n30 = n27 & n28 ;
  assign n31 = ~n29 & ~n30 ;
  assign n32 = n26 & ~n31 ;
  assign n33 = ~n26 & n31 ;
  assign n34 = ~n32 & ~n33 ;
  assign n35 = n25 & ~n34 ;
  assign n36 = ~n25 & n34 ;
  assign n37 = ~n35 & ~n36 ;
  assign n38 = ~n9 & n18 ;
  assign n39 = ~n19 & ~n38 ;
  assign n40 = n37 & ~n39 ;
  assign n41 = ~n24 & n40 ;
  assign n42 = ~n26 & ~n30 ;
  assign n43 = ~n29 & ~n42 ;
  assign n44 = ~n35 & ~n43 ;
  assign n45 = n24 & ~n40 ;
  assign n46 = ~n44 & ~n45 ;
  assign n47 = ~n41 & ~n46 ;
  assign n48 = x0 & ~x1 ;
  assign n49 = x6 & ~x7 ;
  assign n50 = x4 & ~x5 ;
  assign n51 = x2 & ~x3 ;
  assign n52 = ~n50 & ~n51 ;
  assign n53 = n50 & n51 ;
  assign n54 = ~n52 & ~n53 ;
  assign n55 = n49 & ~n54 ;
  assign n56 = ~n49 & n54 ;
  assign n57 = ~n55 & ~n56 ;
  assign n58 = n48 & ~n57 ;
  assign n59 = ~n49 & ~n53 ;
  assign n60 = ~n52 & ~n59 ;
  assign n61 = n58 & n60 ;
  assign n62 = ~x4 & ~x5 ;
  assign n63 = ~x2 & ~x3 ;
  assign n64 = ~n62 & ~n63 ;
  assign n65 = ~x6 & ~x7 ;
  assign n66 = n62 & n63 ;
  assign n67 = ~n64 & ~n66 ;
  assign n68 = ~n65 & n67 ;
  assign n69 = ~n64 & ~n68 ;
  assign n70 = ~x0 & ~x1 ;
  assign n71 = n65 & ~n67 ;
  assign n72 = ~n68 & ~n71 ;
  assign n73 = n70 & ~n72 ;
  assign n74 = n69 & n73 ;
  assign n75 = ~n61 & ~n74 ;
  assign n76 = n35 & n43 ;
  assign n77 = ~n22 & ~n76 ;
  assign n78 = n75 & ~n77 ;
  assign n79 = ~n48 & n57 ;
  assign n80 = ~n58 & ~n79 ;
  assign n81 = ~n58 & ~n60 ;
  assign n82 = ~n61 & ~n81 ;
  assign n83 = ~n69 & ~n73 ;
  assign n84 = ~n74 & ~n83 ;
  assign n85 = n82 & ~n84 ;
  assign n86 = ~n70 & n72 ;
  assign n87 = ~n73 & ~n86 ;
  assign n88 = ~n80 & n87 ;
  assign n89 = ~n85 & n88 ;
  assign n90 = n81 & ~n83 ;
  assign n91 = ~n89 & ~n90 ;
  assign n92 = ~n61 & ~n91 ;
  assign n93 = ~n80 & ~n92 ;
  assign n94 = ~n87 & n92 ;
  assign n95 = n37 & ~n47 ;
  assign n96 = n39 & ~n46 ;
  assign n97 = ~n44 & ~n76 ;
  assign n98 = ~n24 & ~n97 ;
  assign n99 = ~n82 & ~n84 ;
  assign n100 = ~n98 & n99 ;
  assign n101 = ~n96 & ~n100 ;
  assign n102 = ~n95 & n101 ;
  assign n103 = ~n94 & n102 ;
  assign n104 = ~n93 & n103 ;
  assign n105 = ~n75 & n77 ;
  assign n106 = n98 & ~n99 ;
  assign n107 = ~n105 & ~n106 ;
  assign n108 = ~n104 & n107 ;
  assign n109 = ~n78 & ~n108 ;
  assign n110 = ~n47 & ~n109 ;
  assign n111 = n92 & n109 ;
  assign n112 = ~n110 & ~n111 ;
  assign y0 = n112 ;
  assign y1 = ~n109 ;
endmodule
