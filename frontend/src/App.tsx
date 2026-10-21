import { Route, Routes } from 'react-router'
import Layout from './components/Layout.tsx'
import CheckEmail from './pages/CheckEmail.tsx'
import Home from './pages/Home.tsx'
import Login from './pages/Login.tsx'
import NotFound from './pages/NotFound.tsx'
import Register from './pages/Register.tsx'

// Only routes whose page exists are declared here.
// Add new routes (profile, browse, chat...) when their feature lands.
function App() {
  return (
    <Routes>
      <Route element={<Layout />}>
        <Route index element={<Home />} />
        <Route path="login" element={<Login />} />
        <Route path="register" element={<Register />} />
        <Route path="register/check-email" element={<CheckEmail />} />
        <Route path="*" element={<NotFound />} />
      </Route>
    </Routes>
  )
}

export default App
