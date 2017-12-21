# Changelog

All notable changes to this project will be documented in this file, following semantic versioning.

## Unreleased

## 1.2.0 - 2017-11-21

### Added

- Functions `pvector2_in_circle`/`vector2_in_circle` to check if 2D vector is inside 2D circle.
- Functions `pvector2_in_triangle`/`vector2_in_triangle` to check if 2D vector is inside 2D triangle.

### Changed

- Quaternion spherical interpolation use `0.95f` as cutoff to switch to linear interpolation.

## 1.1.0 - 2017-11-18

### Added

- Functions `pquaternion_from_2_vectors`/`quaternion_from_2_vectors` to get quaternion from two vectors.
- Functions `pquaternion_null`/`quaternion_null` to get null quaternion.

### Changed

- Fix missing declaration of function to get zero 3D vector.

## 1.0.4 - 2017-11-18

### Changed

- Fix quaternion inverse function.

## 1.0.3 - 2017-11-18

### Changed

- Fix checking if quaternion is zero.

## 1.0.2 - 2017-11-13

### Changed

- Fix typo.

## 1.0.1 - 2017-11-13

### Changed

- Use standard boolean.

## 1.0.0 - 2017-11-13

### Added

- Version macros.

## 0.3.0 - 2017-11-13

### Added

- Functions `pmatrix_linear_interpolation`/`matrix_linear_interpolation` for matrix linear interpolation.
- Functions to test vector and quaternion equality:
  - `pvector2_is_zero`/`vector2_is_zero`
  - `pvector2_is_near_zero`/`vector2_is_near_zero`
  - `pvector2_is_equal`/`vector2_is_equal`
  - `pvector3_is_zero`/`vector3_is_zero`
  - `pvector3_is_near_zero`/`vector3_is_near_zero`
  - `pvector3_is_equal`/`vector3_is_equal`
  - `pquatertion_is_zero`/`quatertion_is_zero`
  - `pquatertion_is_near_zero`/`quatertion_is_near_zero`
  - `pquatertion_is_equal`/`quatertion_is_equal`

## 0.2.0 - 2017-11-11

### Notes

If you are using any of the functions to generate an projection matrices, you should update to one of the newer version. The perspective matrices only chaged in name, but the orthographic matrices changed in implementation, fixing the top-down orientation, and requiring a near and far clipping.

### Added

- Link in the `README.md` to this CHANGELOG file.
- Functions `pmatrix_ortho`/`matrix_ortho` based on the old `pmatrix_ortho_zo`/`matrix_ortho_zo`.
- Functions `pmatrix_perspective`/`matrix_perspective` based on the old `pmatrix_perspective_zo`/`matrix_perspective_zo`.
- Functions `pmatrix_perspective_fov`/`matrix_perspective_fov` based on the old `pmatrix_perspective_fov_zo`/`matrix_perspective_fov_zo`.

### Removed

- Functions `pmatrix_ortho_zo`/`matrix_ortho_zo`.
- Functions `pmatrix_ortho_no`/`matrix_ortho_no`.
- Old functions `pmatrix_ortho`/`matrix_ortho`.
- Functions `pmatrix_perspective_zo`/`matrix_perspective_zo`.
- Functions `pmatrix_perspective_no`/`matrix_perspective_no`.
- Functions `pmatrix_perspective_fov_zo`/`matrix_perspective_fov_zo`.
- Functions `pmatrix_perspective_fov_no`/`matrix_perspective_fov_no`.

## 0.1.0 - 2017-11-11

### Added

- This CHANGELOG file.
