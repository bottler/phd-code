I used the thesis template from Mark Hadley but I show here the style file modified in ways I like.

The changes are (1) that some warnings are fixed - overfull stuff from the pictures on the cover page and fixed the apostrophes to standard ascii ones in the declaration - and (2) that chapter numbers are working in the bookmarks/outline view in the PDF. The latter fix is a bit of a hack, and might not work if you choose a nonstandard numbering system for your chapters - I avoid depending on `\numberline`, because I don't understand why it doesn't work anyway. Just to be clear, you know you need a fix like this if you look at your thesis in a PDF viewer, open the bookmarks or outline sidebar (or whatever) and see "Chapter. Introduction" instead of "Chapter 1. Introduction".

In fact, the version of the template I actually used in my thesis is not exactly this one, but that's due to something I was specifically trying to modify.
