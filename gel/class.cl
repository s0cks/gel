(ns gel
  (defnative get-class [n]
    "Returns the Class with Name [n].")
  (defnative get-classes []
    "Returns the list of Classes registered.")
  (deftype Class
    (defnative get-id [c]
      "Returns the ClassId for Class [c].")
    (defnative is-primitive? [c]
      "Returns true if Class [c] is a primitive Class.")
    (defnative get-fields [c]
      "Returns a list of Fields for Class [c].")
    (defnative get-procedures [c]
      "Returns the Procedures for Class [c].")))