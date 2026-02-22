# API
## &fdrw
### patch related
* -E: patch threshold cost type; -P: coeficient related to cost type (in following, 'p' means the coeficient)
    * not specified, 0: no threshold
    * 1: |patch_node| < p * |abscircuit_node|
    * 2: |patch_node| < p
    * 3: |patch_node| < p * |TFI|
    * 4: |patch_node| < p * |MFFC|
    * 7: patch_height < p * TFI_height
    * 9: |functional_support| < p * |frontier_support|
    * TWOPATH:
        * considered dist = sqrt((dist(PO_rewrite_ckt, node) + dist_in_patch(node, f))^2 + dist(PO_support_ckt, f)^2), with f is merge-frontier support
        * 12: RMS of distances after replace < p * before replace
        * 13: max of distances after replace < p * before replace
        * 15: min of distances after replace < p * before replace
    * SUPPATH:
        * considered dist = dist(PO_rewrite_ckt, node) + dist_in_patch(node, f), with f is merge-frontier support
        * 10: RMS of distances after replace < p * before replace
        * 11: max of distances after replace < p * before replace
        * 14: min of distances after replace < p * before replace
* -g: consider merge frontier in functional support
* -G: functional support setting
    * 0: nodes with subset merge-frontier support in support circuit
    * 1: higher nodes in support circuit
    * 2: all nodes in support circuit
    * 3: higher nodes in support circuit + shared part exclude TFI
    * 4: all nodes in support circuit + shared part exclude TFI
    * 5: lower nodes in support circuit
    * 6: lower nodes in support circuit + shared part exclude TFI
### level related
* -a: compute level based on abstract circuit
* -m: use minimum path length for level computation
* -p: compute level start from FI
* -s: use shortest path for distance
### others
* -C: conflict limit for finding patch
* -N: single control signal
    * N < 0: special mode, usually -F is better choice
    * N > 0: compute the patch for single node, with -N specify the node id
    * not specified, also -F not given: compute the patch for all nodes in abstract circuit
* -F: controlling file
    * format
        * all numbers should in integer form
        * list the numbers in a line, with ',' seperating the numbers
        * last number should be followed with '\n', not ',' allowed
        * i.e.
        ```
        (number1),(number2),...,(lastNumber)
        
        ```
* -S: the cared nodes
    * format
        * each line start with 'f'
        * each line at most 10 numbers
        * each numbers and 'f' should be separated with ' '
        * last line should be empty
        * i.e.
        ```
        f (number1) (number2) ... (number10)
        f (number11) (number12) ...
        ...
        
        ```
## experiments
* the experimental result is in the directory "experiments", it contains the results pasted in the thesis
* To do the same or similar experiment, here list the environment settings
### run cec
* abc -c "&read (aig); &cec -m -v"
### check all
* abc -c "&read (aig); &fdrw \[-g\] \[-G number\] -v -w -C (number)"
### aggresive bottom-up
* abc -c "&read (aig); &fdrw -G 2 -a -F (coef_file) -C (number) \[-S cared_node_file\]"
* coef file:
    * -12,(1 or 2),(limLv),(minLv),(maxLv)
        * check all nodes in circuit 1 (or 2) with level in \[minLv,maxLv\] and support level >= limLv
    * -12,0,(limLv),(nodeId1),(nodeId2)...
        * compute patches of nodes nodeId1, nodeId2... with support level >= limLv, and replace the nodes with patch
    * -12,3,(limLv)
        * check the nodes in cared_node_file with support level >= limLv. If no -S, then all nodes in abstract circuit is checked
### level based bottom-up
* single iteration
    * "&read (aig); &fdrw -g -G 5 -a -F (coef file) -C (number)"
    * coef file: -14,(1 or 2),(minLv),(maxLv),(ratio)
        * rewrite nodes in circuit 1 (or 2) with level within \[minLv, maxLv\], with patch size < ratio * (TFI + 5)
* iteratively run
    1. make a directory, cd the directory
    2. copy 'run\.py' in 'experiments/level_botup'
    3. cmd: mkdir 0
    4. put the initial aig file in 'directory/0', named as '_init.aig'
    5. modify run\.py
        * (line 10: coef = \[...\]) change the coef to wanted one
        * change all "~/abc_fdrw/abc" into correct allocation of abc execution file
    6. cmd: python run\.py (directory)
### greedy bottom-up
* single iteration
    * "&read (aig); &fdrw -g -G 2 -a -F (coef file) -C (number)"
    * coef file: -15,(1 or 2)
        * rewrite nodes in circuit 1 (or 2)
* iteratively run
    1. make a directory, cd the directory
    2. copy 'run\.py' in 'experiments/level_botup'
    3. cmd: mkdir 0
    4. put the initial aig file in 'directory/0', named as '_init.aig'
    5. modify run\.py
        * change all "~/abc_fdrw/abc" into correct allocation of abc execution file
    6. cmd: python run\.py (directory)
### conflict-based bottom-up
* single iteration
    * "&read (aig); &fdrw -g -G 2 \[-E number -P 70.0\] -a -F (coef file) -C (number)"
    * coef file: -7,(1 or 2),0
        * rewrite nodes in circuit 1 (or 2)
* iteratively run
    1. make a directory, cd the directory
    2. copy 'run\.py' in 'experiments/level_botup'
    3. cmd: mkdir 0
    4. put the initial aig file in 'directory/0', named as 'final\.aig'
    5. make file 'info\.log' in 'directory/0', with following format:
        ```
        ckt:(1 or 2) // opposite circuit of the first rewrite circuit
        partSyn:0
        Result:(directory)/0/final.aig
        C:(conflict limit)
        basicCmd:-g -G 2 -a (other constraint required)
        ```
    6. modify runcmd\.py
        * change all "~/abc_fdrw/abc" into correct allocation of abc execution file
    7. cmd: python run\.py (directory)