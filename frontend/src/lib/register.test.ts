import { describe, expect, test } from 'vitest'
import {
  failedPasswordRules,
  serverErrorField,
  toPayload,
  validateRegister,
  type RegisterForm,
} from './register.ts'

const valid: RegisterForm = {
  username: 'alice',
  email: 'alice@example.com',
  first_name: 'Alice',
  last_name: 'Liddell',
  password: 'Wonder1and',
  confirm_password: 'Wonder1and',
}

describe('failedPasswordRules', () => {
  test('accepts a strong password', () => {
    expect(failedPasswordRules('Wonder1and', 'alice')).toEqual([])
  })

  test.each([
    ['Ab1', 'length'],
    ['ABCDEFG1', 'lower'],
    ['abcdefg1', 'upper'],
    ['Abcdefgh', 'digit'],
    ['MyPassword1', 'common'],
    ['Qwerty123', 'common'],
  ])('%s fails the %s rule', (password, rule) => {
    expect(failedPasswordRules(password).map((r) => r.id)).toContain(rule)
  })

  test('rejects a password containing the username, case-insensitively', () => {
    expect(failedPasswordRules('XAlice2024', 'alice').map((r) => r.id)).toEqual(['common'])
  })

  test('ignores an empty username', () => {
    expect(failedPasswordRules('Wonder1and', '   ')).toEqual([])
  })
})

describe('validateRegister', () => {
  test('returns no errors for a valid form', () => {
    expect(validateRegister(valid)).toEqual({})
  })

  test('flags every empty field', () => {
    const errors = validateRegister({
      username: ' ',
      email: '',
      first_name: '',
      last_name: '  ',
      password: '',
      confirm_password: '',
    })
    expect(errors).toEqual({
      username: 'Username is required',
      email: 'Email is required',
      first_name: 'First name is required',
      last_name: 'Last name is required',
      password: 'Password is required',
      confirm_password: 'Please confirm your password',
    })
  })

  test('rejects a malformed email', () => {
    expect(validateRegister({ ...valid, email: 'alice@' }).email).toBe('Enter a valid email address')
  })

  test('rejects a username with spaces', () => {
    expect(validateRegister({ ...valid, username: 'al ice' }).username).toBe('Username cannot contain spaces')
  })

  test('lists the unmet password rules', () => {
    const errors = validateRegister({ ...valid, password: 'abc', confirm_password: 'abc' })
    expect(errors.password).toBe(
      'Password needs: at least 8 characters, an uppercase letter, a number',
    )
  })

  test('rejects a confirmation that does not match', () => {
    expect(validateRegister({ ...valid, confirm_password: 'Wonder1ane' }).confirm_password).toBe(
      'Passwords do not match',
    )
  })
})

describe('toPayload', () => {
  test('trims identity fields, keeps the password as-is and drops the confirmation', () => {
    expect(
      toPayload({ ...valid, username: ' alice ', email: ' alice@example.com', password: ' Wonder1and ' }),
    ).toEqual({
      username: 'alice',
      email: 'alice@example.com',
      first_name: 'Alice',
      last_name: 'Liddell',
      password: ' Wonder1and ',
    })
  })
})

describe('serverErrorField', () => {
  test.each([
    ['Username already exists', 'username'],
    ['Email already exists', 'email'],
    ['Username is required', 'username'],
    ['Email is required', 'email'],
    ['Password is required', 'password'],
    ['First name is required', 'first_name'],
    ['Last name is required', 'last_name'],
  ])('%s → %s', (message, field) => {
    expect(serverErrorField(message)).toBe(field)
  })

  test.each(['Invalid JSON', 'Failed to register user', 'Request failed (502)'])(
    '%s stays form-level',
    (message) => {
      expect(serverErrorField(message)).toBeUndefined()
    },
  )
})
